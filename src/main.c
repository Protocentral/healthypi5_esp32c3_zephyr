#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/sys/ring_buffer.h>

#include "hpi_data_service.h"
#include "cmd_module.h"

LOG_MODULE_REGISTER(ble_backend);

struct bt_conn *current_conn;

#define MSG_SIZE 32

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_ALIAS(rp_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
				  BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))};

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++)
	{
		//printk("S %c\n", buf[i]);
		uart_poll_out(uart_dev, buf[i]);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		printk("Connection failed (err 0x%02x)\n", err);
	}
	else
	{
		printk("Connected\n");
		send_status_serial(BLE_STATUS_CONNECTED);
		current_conn = bt_conn_ref(conn);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
	send_status_serial(BLE_STATUS_DISCONNECTED);
	if (current_conn)
	{
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	err = wiser_service_init();

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

void remove_separators(char* str) {
    char *pr = str, *pw = str;
	char c=':';
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

void set_ble_name()
{
	bt_addr_le_t addrs;
	size_t count;

	char addr_str[50];
	size_t add_str_len;
	char dev_name[30] = "HPI5-";

	bt_id_get(&addrs, &count);

	bt_addr_le_to_str(&addrs, addr_str, 10);
	remove_separators(addr_str);
	strcat(dev_name, addr_str);
	bt_set_name(dev_name);
}


void main(void)
{
	int err;

	k_sleep(K_SECONDS(5));

	if (!device_is_ready(uart_dev))
	{
		printk("UART device not found!");
		return;
	}

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	set_ble_name();
	bt_ready();
	bt_conn_auth_cb_register(&auth_cb_display);

	// start_adv();

	while (1)
	{
		
		k_sleep(K_MSEC(1000));
	}
}
