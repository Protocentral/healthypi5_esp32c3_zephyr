#pragma once

#include <stdint.h>
#include <zephyr/kernel.h>

#define SMP_VERSION 0x0

enum smp_uart_op_t{
    MGMT_OP_READ = 0x0,
    MGMT_OP_READ_RSP = 0x1,
    MGMT_OP_WRITE = 0x2,
    MGMT_OP_WRITE_RSP = 0x3,
};

struct smp_header{
    uint8_t res:3;
    uint8_t ver:2;
    uint8_t op:3;

    uint8_t flags;
    uint8_t len;
    uint8_t group;
    uint8_t seq;
    uint8_t id;
};

struct smp_packet{
    struct smp_header header;
    uint8_t payload[0];
};