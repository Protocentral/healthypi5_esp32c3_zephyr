#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>

#include "hpi_data_service.h"
#include "cmd_module.h"
#include "pc_smp_uart_client.h"
#include <zephyr/drivers/led.h>

#include "hw_module.h"

LOG_MODULE_REGISTER(cmd_module);

#define MAX_MSG_SIZE 32

#define UART_DEVICE_NODE DT_ALIAS(rp_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static volatile int ecs_rx_state = 0;

int cmd_pkt_len;
int cmd_pkt_pos_counter, cmd_pkt_data_counter;
int cmd_pkt_pkttype;

uint8_t ces_pkt_data_buffer[1000]; // = new char[1000];

volatile bool cmd_module_ble_connected = false;

K_SEM_DEFINE(sem_ble_connected, 0, 1);
K_SEM_DEFINE(sem_ble_disconnected, 0, 1);

struct hpi_cmd_data_obj_t
{
    void *fifo_reserved; /* 1st word reserved for use by FIFO */

    uint8_t pkt_type;
    uint8_t data_len;
    uint8_t data[MAX_MSG_SIZE];
};

K_FIFO_DEFINE(cmd_data_fifo);

struct hpi_cmd_data_obj_t cmd_data_obj;

void ces_parse_packet(char rxch)
{
    // printk("%0x\n", rxch);

    switch (ecs_rx_state)
    {
    case CMD_SM_STATE_INIT:
        if (rxch == CES_CMDIF_PKT_START_1)
            ecs_rx_state = CMD_SM_STATE_SOF1_FOUND;
        break;

    case CMD_SM_STATE_SOF1_FOUND:
        if (rxch == CES_CMDIF_PKT_START_2)
            ecs_rx_state = CMD_SM_STATE_SOF2_FOUND;
        else
            ecs_rx_state = CMD_SM_STATE_INIT; // Invalid Packet, reset state to init
        break;

    case CMD_SM_STATE_SOF2_FOUND:

        ecs_rx_state = CMD_SM_STATE_PKTLEN_FOUND;
        cmd_pkt_len = (int)rxch;
        cmd_pkt_pos_counter = CES_CMDIF_IND_LEN;
        cmd_pkt_data_counter = 0;
        break;

    case CMD_SM_STATE_PKTLEN_FOUND:

        cmd_pkt_pos_counter++;
        if (cmd_pkt_pos_counter < CES_CMDIF_PKT_OVERHEAD) // Read Header
        {
            if (cmd_pkt_pos_counter == CES_CMDIF_IND_LEN_MSB)
                cmd_pkt_len = (int)((rxch << 8) | cmd_pkt_len);
            else if (cmd_pkt_pos_counter == CES_CMDIF_IND_PKTTYPE)
            {
                cmd_pkt_pkttype = (int)rxch;
                ces_pkt_data_buffer[cmd_pkt_data_counter++] = (char)(rxch);
            }
        }
        else if ((cmd_pkt_pos_counter >= CES_CMDIF_PKT_OVERHEAD) && (cmd_pkt_pos_counter < CES_CMDIF_PKT_OVERHEAD + cmd_pkt_len + 1)) // Read Data
        {
            ces_pkt_data_buffer[cmd_pkt_data_counter++] = (char)(rxch); // Buffer that assigns the data separated from the packet
        }
        else // All data received
        {
            if (rxch == CES_CMDIF_PKT_STOP_2)
            {
                printk("Packet Received len: %d, type: %d\n", cmd_pkt_len, cmd_pkt_pkttype);
                cmd_pkt_pos_counter = 0;
                cmd_pkt_data_counter = 0;
                ecs_rx_state = 0;
                cmd_data_obj.pkt_type = cmd_pkt_pkttype;
                cmd_data_obj.data_len = cmd_pkt_len + 1; //+1 for the packet type

                memcpy(cmd_data_obj.data, ces_pkt_data_buffer, cmd_pkt_len + 1);

                k_fifo_put(&cmd_data_fifo, &cmd_data_obj);
            }
        }
    }
}

void hpi_decode_progress_packet(uint8_t *in_pkt_buf, uint8_t pkt_len)
{
    uint16_t prog_time = in_pkt_buf[0] | (in_pkt_buf[1] << 8);
    uint16_t prog_current = in_pkt_buf[2] | (in_pkt_buf[3] << 8);
    uint16_t prog_imped = in_pkt_buf[4] | (in_pkt_buf[5] << 8);

    printk("Progress recd: %d time: %d current: %d imped: %d\n", pkt_len, prog_time, prog_current, prog_imped);
    send_progress_ble(prog_time, prog_current, prog_imped);
}

void cmdif_send_uart(char *buf, uint8_t buf_size)
{
    // int msg_len = strlen(buf);

    for (int i = 0; i < buf_size; i++)
    {
        // printk("S %x\n", buf[i]);
        uart_poll_out(uart_dev, buf[i]);
        k_sleep(K_USEC(100));
    }
}

void cmdif_send_ble_progress(uint16_t m_time, uint16_t m_current, uint16_t m_imped)
{
    uint8_t cmd_pkt[11];
    cmd_pkt[0] = CES_CMDIF_PKT_START_1;
    cmd_pkt[1] = CES_CMDIF_PKT_START_2;
    cmd_pkt[2] = 0x04;
    cmd_pkt[3] = 0x00;
    cmd_pkt[4] = CES_CMDIF_TYPE_PROGRESS;
    cmd_pkt[5] = (uint8_t)(m_time & 0x00FF);
    cmd_pkt[6] = (uint8_t)((m_time >> 8) & 0x00FF);
    cmd_pkt[7] = (uint8_t)(m_current & 0x00FF);
    cmd_pkt[8] = (uint8_t)((m_current >> 8) & 0x00FF);
    cmd_pkt[9] = CES_CMDIF_PKT_STOP_1;
    cmd_pkt[10] = CES_CMDIF_PKT_STOP_2;

    for (int i = 0; i < 11; i++)
    {
        uart_poll_out(uart_dev, cmd_pkt[i]);
    }
}

void cmdif_send_ble_command(uint8_t m_cmd)
{
    uint8_t cmd_pkt[6];
    cmd_pkt[0] = CES_CMDIF_PKT_START_1;
    cmd_pkt[1] = CES_CMDIF_PKT_START_2;
    cmd_pkt[2] = 0x01;
    cmd_pkt[3] = 0x00;
    cmd_pkt[4] = CES_CMDIF_TYPE_CMD;
    cmd_pkt[4] = m_cmd;
    cmd_pkt[5] = CES_CMDIF_PKT_STOP_1;
    cmd_pkt[6] = CES_CMDIF_PKT_STOP_2;

    for (int i = 0; i < 6; i++)
    {
        uart_poll_out(uart_dev, cmd_pkt[i]);
    }
}

void send_data_serial(uint8_t *in_data_buf, uint8_t in_data_len)
{
    uint8_t dataPacket[50];

    dataPacket[0] = CES_CMDIF_PKT_START_1;
    dataPacket[1] = CES_CMDIF_PKT_START_2;
    dataPacket[2] = in_data_len;
    dataPacket[3] = 0;
    dataPacket[4] = CES_CMDIF_TYPE_DATA;

    for (int i = 0; i < in_data_len; i++)
    {
        dataPacket[i + 5] = in_data_buf[i];
        printk("Data %x: %d\n", i, in_data_buf[i]);
    }

    dataPacket[in_data_len + 5] = CES_CMDIF_PKT_STOP_1;
    dataPacket[in_data_len + 6] = CES_CMDIF_PKT_STOP_2;

    printk("Sending UART data: %d\n", in_data_len);

    cmdif_send_uart(dataPacket, in_data_len + 7);
}



void send_cmdif_cmd_reset_rp(void)
{
    uint8_t dataPacket[10];

    dataPacket[0] = CES_CMDIF_PKT_START_1;
    dataPacket[1] = CES_CMDIF_PKT_START_2;
    dataPacket[2] = 1;
    dataPacket[3] = 0;
    dataPacket[4] = CES_CMDIF_TYPE_CMD;

    dataPacket[5] = HPI_CMD_RESET;

    dataPacket[6] = CES_CMDIF_PKT_STOP_1;
    dataPacket[7] = CES_CMDIF_PKT_STOP_2;

    printk("Sending CMD Reset\n");

    cmdif_send_uart(dataPacket, 8);
}

void send_status_serial(uint8_t in_status_buf)
{
    uint8_t dataPacket[50];

    dataPacket[0] = CES_CMDIF_PKT_START_1;
    dataPacket[1] = CES_CMDIF_PKT_START_2;
    dataPacket[2] = 1;
    dataPacket[3] = 0;
    dataPacket[4] = CES_CMDIF_TYPE_STATUS;
    dataPacket[5] = in_status_buf;
    dataPacket[6] = CES_CMDIF_PKT_STOP_1;
    dataPacket[7] = CES_CMDIF_PKT_STOP_2;

    printk("Sending status %d \n", in_status_buf);
    cmdif_send_uart(dataPacket, 8);
}

void cmd_serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(uart_dev))
    {
        return;
    }

    if (!uart_irq_rx_ready(uart_dev))
    {
        return;
    }

    while (uart_fifo_read(uart_dev, &c, 1) == 1)
    {
        ces_parse_packet(c);
        // printk("R: %X\n", c);
    }
}

static void cmd_init(void)
{
    int ret=0;

    printk("CMD Module Init\n");

    if (!device_is_ready(uart_dev))
    {
        printk("UART device not found!");
        return;
    }

    /* configure interrupt and callback to receive data */
    ret = uart_irq_callback_user_data_set(uart_dev, cmd_serial_cb, NULL);

    if (ret < 0)
    {
        if (ret == -ENOTSUP)
        {
            printk("Interrupt-driven UART API support not enabled\n");
        }
        else if (ret == -ENOSYS)
        {
            printk("UART device does not support interrupt-driven API\n");
        }
        else
        {
            printk("Error setting UART callback: %d\n", ret);
        }
        return;
    }
    uart_irq_rx_enable(uart_dev);

    // smp_mcu_reset();
    // pc_smp_image_upload();
    
    k_sleep(K_MSEC(5000));
    send_cmdif_cmd_reset_rp();
    //pc_smp_get_image_state();
}

void cmd_thread(void)
{
    struct hpi_cmd_data_obj_t *rx_cmd_data_obj;

    // k_sleep(K_MSEC(5000));
    printk("CMD Thread Started\n");

    cmd_init();

    for (;;)
    {
        rx_cmd_data_obj = k_fifo_get(&cmd_data_fifo, K_FOREVER);
        printk("Recd Data: %d Type: %d \n", rx_cmd_data_obj->data_len, rx_cmd_data_obj->pkt_type);

        if (rx_cmd_data_obj->pkt_type == CES_CMDIF_TYPE_DATA)
        {
            // hpi_decode_data_packet(rx_cmd_data_obj->data, rx_cmd_data_obj->data_len);
        }
        else if (rx_cmd_data_obj->pkt_type == CES_CMDIF_TYPE_STATUS)
        {
            send_data_ble(rx_cmd_data_obj->data, rx_cmd_data_obj->data_len);
        }
        else if (rx_cmd_data_obj->pkt_type == CES_CMDIF_TYPE_PROGRESS)
        {
            send_data_ble(rx_cmd_data_obj->data, rx_cmd_data_obj->data_len);
            // hpi_decode_progress_packet(rx_cmd_data_obj->data, rx_cmd_data_obj->data_len);
            //  printk("Recd Progress Data\n");
            //  hpi_decode_command(rx_cmd_data_obj->data[0]);
        }
        else
        {
            printk("Recd Unknown Data\n");
        }



        k_sleep(K_MSEC(1000));
    }
}

#define CMD_THREAD_STACKSIZE 4096
#define CMD_THREAD_PRIORITY 7

K_THREAD_DEFINE(cmd_thread_id, CMD_THREAD_STACKSIZE, cmd_thread, NULL, NULL, NULL, CMD_THREAD_PRIORITY, 0, 0);
