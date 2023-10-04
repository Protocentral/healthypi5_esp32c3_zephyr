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

#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_client.h>

#include "hw_module.h"

// #include "pc_smp_uart_client.h"

#define UART_MTU_SIZE 128
#define UART_DEVICE_NODE DT_ALIAS(rp_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

uint8_t smp_seq_num = 0;

static struct smp_client_object smp_client;
static struct img_mgmt_client img_client;
static struct os_mgmt_client os_client;
static struct mcumgr_image_data image_info[2];

#define TEST_IMAGE_NUM 1
#define TEST_IMAGE_SIZE 2048
#define TEST_SLOT_NUMBER 2

/* IMG group data */
static uint8_t image_hash[32];
static struct mcumgr_image_data image_info[2];
static uint8_t image_dummy[1024];

static struct net_buf *buf[5];

static struct smp_transport smpt_test;
static struct smp_client_transport_entry smp_client_transport;
struct mcumgr_image_state res_buf;

#define SMP_RESPONSE_WAIT_TIME 3

/* Test os_mgmt echo command with 40 bytes of data: "short MCUMGR test application message..." */
static const uint8_t command[] = {
    0x02,
    0x00,
    0x00,
    0x2e,
    0x00,
    0x00,
    0x01,
    0x00,
    0xbf,
    0x61,
    0x64,
    0x78,
    0x28,
    0x73,
    0x68,
    0x6f,
    0x72,
    0x74,
    0x20,
    0x4d,
    0x43,
    0x55,
    0x4d,
    0x47,
    0x52,
    0x20,
    0x74,
    0x65,
    0x73,
    0x74,
    0x20,
    0x61,
    0x70,
    0x70,
    0x6c,
    0x69,
    0x63,
    0x61,
    0x74,
    0x69,
    0x6f,
    0x6e,
    0x20,
    0x6d,
    0x65,
    0x73,
    0x73,
    0x61,
    0x67,
    0x65,
    0x2e,
    0x2e,
    0x2e,
    0xff,
};

static uint16_t smp_uart_get_mtu(const struct net_buf *nb)
{
    return 256;
}

static int smp_uart_tx_pkt(struct net_buf *nb)
{
    smp_packet_free(nb);
    return 0;
}

/*void stub_smp_client_transport_register(void)
{

    smpt_test.functions.output = smp_uart_tx_pkt;
    smpt_test.functions.get_mtu = smp_uart_get_mtu;

    smp_transport_init(&smpt_test);
    smp_client_transport.smpt = &smpt_test;
    smp_client_transport.smpt_type = SMP_SERIAL_TRANSPORT;
    smp_client_transport_register(&smp_client_transport);
}*/

static void setup_smp_client(void)
{
    smp_client_object_init(&smp_client, SMP_SERIAL_TRANSPORT);
    os_mgmt_client_init(&os_client, &smp_client);
    img_mgmt_client_init(&img_client, &smp_client, 2, image_info);
}

void pc_smp_image_upload(void)
{
    int rc = 0;
    struct mcumgr_image_upload response;

    setup_smp_client();
    rc = img_mgmt_client_upload_init(&img_client, TEST_IMAGE_SIZE, TEST_IMAGE_NUM, image_hash);

    rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
}

void pc_smp_get_image_state(void)
{
    int rc = 0;

    setup_smp_client();

    rc = img_mgmt_client_state_read(&img_client, &res_buf);

    printk("Image state: %d\n", res_buf.image_list_length);

    if (res_buf.image_list_length == 0)
    {
        int i = 0;
        printk("Single Image %d: %d %s %s \n", i, res_buf.image_list[i].img_num, res_buf.image_list[i].hash, res_buf.image_list[i].version);
    }
    for (int i = 0; i < res_buf.image_list_length; i++)
    {
        printk("Image %d: %d %s %s \n", i, res_buf.image_list[i].img_num, res_buf.image_list[i].hash, res_buf.image_list[i].version);
    }
}

void smp_mcu_reset(void)
{
    int rc;

    printk("\nStarting SMP Client...\n");

    setup_smp_client();

    printk("Resetting device...\n");

    rc = os_mgmt_client_reset(&os_client);
    if (rc != 0)
    {
        printk("Error: %d\n", rc);
    }
    else
    {
        printk("Reset successful\n");
    }
}

void pc_smp_boot_rp(void)
{
    rp_set_boot_ctrl_on();
    k_sleep(K_MSEC(500));
    send_cmdif_cmd_reset_rp();
    k_sleep(K_MSEC(500));

    // Device should now be in Serial Recovery mode

    // pc_smp_get_image_state();
}

/*void smp_uart_send_cmd(uint8_t group_id, uint8_t op_id, uint8_t cmd_id)
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
*/

char enc_buffer[UART_MTU_SIZE];

void smp_uart_send_uart(uint8_t *buf, uint8_t buf_size)
{
    int rc;
    uint8_t olen = 0;

    rc = base64_encode(enc_buffer, sizeof(enc_buffer), &olen, buf, buf_size);
    if (rc < 0)
    {
        printk("Error encoding base64\n");
        return;
    }

    printk("Encoded: %s\n", enc_buffer);
    printk("Encoded len: %d\n", olen);

    // uart_poll_out(uart_dev, enc_buffer);
}
