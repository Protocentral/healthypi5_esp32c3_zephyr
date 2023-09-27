#pragma once

#include <stdint.h>
#include <zephyr/kernel.h>

#define SMP_VERSION 0x0

/**
 * MCUmgr groups. The first 64 groups are reserved for system level mcumgr
 * commands. Per-user commands are then defined after group 64.
 */
enum mcumgr_group_t {
	/** OS (operating system) group */
	MGMT_GROUP_ID_OS	= 0,

	/** Image management group, used for uploading firmware images */
	MGMT_GROUP_ID_IMAGE,

	/** Statistic management group, used for retieving statistics */
	MGMT_GROUP_ID_STAT,

	/** Settings management (config) group, used for reading/writing settings */
	MGMT_GROUP_ID_SETTINGS,

	/** Log management group (unused) */
	MGMT_GROUP_ID_LOG,

	/** Crash group (unused) */
	MGMT_GROUP_ID_CRASH,

	/** Split image management group (unused) */
	MGMT_GROUP_ID_SPLIT,

	/** Run group (unused) */
	MGMT_GROUP_ID_RUN,

	/** FS (file system) group, used for performing file IO operations */
	MGMT_GROUP_ID_FS,

	/** Shell management group, used for executing shell commands */
	MGMT_GROUP_ID_SHELL,

	/** User groups defined from 64 onwards */
	MGMT_GROUP_ID_PERUSER	= 64,

	/** Zephyr-specific groups decrease from PERUSER to avoid collision with upstream and
	 *  user-defined groups.
	 *  Zephyr-specific: Basic group
	 */
	ZEPHYR_MGMT_GRP_BASIC	= (MGMT_GROUP_ID_PERUSER - 1),
};

/** Opcodes; encoded in first byte of header. */
enum mcumgr_op_t {
	MGMT_OP_READ		= 0,
	MGMT_OP_READ_RSP,
	MGMT_OP_WRITE,
	MGMT_OP_WRITE_RSP,
};

/**
 * Command IDs for OS management group.
 
#define OS_MGMT_ID_ECHO			0
#define OS_MGMT_ID_CONS_ECHO_CTRL	1
#define OS_MGMT_ID_TASKSTAT		2
#define OS_MGMT_ID_MPSTAT		3
#define OS_MGMT_ID_DATETIME_STR		4
#define OS_MGMT_ID_RESET		5
#define OS_MGMT_ID_MCUMGR_PARAMS	6
#define OS_MGMT_ID_INFO			7

struct smp_header_t{
    uint8_t res:3;
    uint8_t ver:2;
    uint8_t op:3;

    uint8_t flags;
    uint8_t len;
    uint8_t group;
    uint8_t seq;
    uint8_t id;
};

struct smp_packet_t {
    struct smp_header_t header;
    uint8_t payload;
};
*/
void smp_uart_send_uart(uint8_t *buf, uint8_t buf_size);
void smp_uart_send_cmd(uint8_t group_id, uint8_t op_id, uint8_t cmd_id);
void pc_smp_image_upload(void);
void pc_smp_get_image_state(void);
