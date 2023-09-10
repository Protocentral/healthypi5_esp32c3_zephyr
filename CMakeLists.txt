# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tdcs4_esp32c3)

target_sources(app PRIVATE
    src/main.c
    src/hpi_data_service.c
    src/cmd_module.c
)
