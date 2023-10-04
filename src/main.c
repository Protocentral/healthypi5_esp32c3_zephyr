#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/sys/ring_buffer.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

#include "hpi_data_service.h"
#include "cmd_module.h"
#include "ble_module.h"
#include "fs_module.h"

#include <zephyr/mgmt/mcumgr/smp/smp_client.h>

LOG_MODULE_REGISTER(ble_backend);

void main(void)
{
	int rc;

	printk("HealthyPi App started!\n");

	// Delay introduced only to allow logs to be printed on USB CDC
	//k_sleep(K_SECONDS(5));

	//fs_module_init();

	//Init BLE module
	//ble_module_init();

	
	while (1)
	{
		k_sleep(K_MSEC(1000));
	}
}

