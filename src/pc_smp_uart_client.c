/*
 * Copyright ProtoCentral Electronics 2023. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Custom SMP Client to program another Zephyr device through SMP MCU Manager
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/sys/base64.h>

#include "pc_smp_uart_client.h"

#define UART_MTU_SIZE 128
#define UART_DEVICE_NODE DT_ALIAS(rp_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

uint8_t smp_seq_num = 0;

void smp_mcu_reset(void)
{
    /*int rc;

    smp_client_response_buf_clean();
    smp_stub_set_rx_data_verify(NULL);

    smp_client_send_status_stub(MGMT_ERR_EOK);
    /* Test timeout
    rc = os_mgmt_client_reset(&os_client);
    zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
                  MGMT_ERR_ETIMEOUT, rc);
    /* Testing reset successfully handling 
    os_reset_response();
    rc = os_mgmt_client_reset(&os_client);
    zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);]

    */
    printk("Resetting device\n");
    smp_uart_send_cmd(MGMT_GROUP_ID_OS, OS_MGMT_ID_RESET, MGMT_OP_WRITE);
}

void smp_uart_send_cmd(uint8_t group_id, uint8_t op_id, uint8_t cmd_id)
{
    uint8_t pkt_buffer[UART_MTU_SIZE];

    struct smp_packet_t *pkt = (struct smp_packet_t *)pkt_buffer;
    //pkt.header.res = 0;
    pkt->header.group = group_id;
    pkt->header.op = op_id;
    pkt->header.id = cmd_id;

    pkt->header.ver=0;
    pkt->header.flags=0;
    pkt->header.len=0;

    printk("Sending cmd: %X\n", cmd_id);

    for(int i=0; i<sizeof(struct smp_packet_t); i++)
        printk("%X ", pkt_buffer[i]);
    printk("\n");
    smp_uart_send_uart(pkt_buffer, sizeof(struct smp_packet_t));
}

char enc_buffer[UART_MTU_SIZE];

void smp_uart_send_uart(uint8_t *buf, uint8_t buf_size)
{
    int rc;
    uint8_t olen=0;

    rc = base64_encode(enc_buffer, sizeof(enc_buffer), &olen, buf, buf_size);
    if (rc < 0) {
        printk("Error encoding base64\n");
        return;
    }

    printk("Encoded: %s\n", enc_buffer);
    printk("Encoded len: %d\n", olen);

    //uart_poll_out(uart_dev, enc_buffer);

}
