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
uint8_t enc_buffer[UART_MTU_SIZE];

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
}

void smp_uart_send_cmd(uint8_t group_id, uint8_t op_id, uint8_t cmd_id)
{
   uint8_t pkt_buffer[]
}

void smp_uart_send_uart(uint8_t *buf, uint8_t buf_size)
{
    int rc;
	const unsigned char *src;

    src = buf;
    rc = base64_encode(enc_buffer, sizeof(enc_buffer), &src, buf_size);
    if (rc < 0) {
        printk("Error encoding base64\n");
        return;
    }

    printk("Encoded: %s\n", enc_buffer);
    uart_poll_out(uart_dev, enc_buffer, rc);

}
