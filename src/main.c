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

#include <zephyr/mgmt/mcumgr/smp/smp_client.h>

LOG_MODULE_REGISTER(ble_backend);

#define STORAGE_PARTITION_LABEL storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION_LABEL)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)STORAGE_PARTITION_ID,
	.mnt_point = "/lfs1"};

void main(void)
{
	int rc;

	// Delay introduced only to allow logs to be printed on USB CDC
	//k_sleep(K_SECONDS(5));

	rc = fs_mount(&littlefs_mnt);
	if (rc < 0)
	{
		LOG_ERR("Error mounting littlefs [%d]", rc);
	}

	//Init BLE module
	ble_module_init();

	printk("HeatlhyPi App started!\n");

	while (1)
	{
		k_sleep(K_MSEC(1000));
	}
}
