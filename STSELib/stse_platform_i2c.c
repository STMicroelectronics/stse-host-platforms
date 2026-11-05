/**
 ******************************************************************************
 * @file    stse_platform_i2c.c
 * @brief   STSecureElement I2C platform (source) - True Zero-Copy Scatter-Gather
 * @author  STMicroelectronics - CS application team
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include "core/stse_platform.h"
#include "drivers/i2c/I2C.h"

stse_return_code_t stse_platform_i2c_init(PLAT_UI8 bus_id, void *p_arg) {
    (void)bus_id;
    (void)p_arg;
    return (stse_return_code_t)i2c_init(I2C1);
}

stse_return_code_t stse_platform_i2c_wake(PLAT_UI8 bus_id,
                                         PLAT_UI8 dev_addr,
                                         PLAT_UI16 speed) {
    (void)bus_id;
    (void)speed;

    i2c_wake(I2C1, dev_addr);
    return STSE_OK;
}

stse_return_code_t stse_platform_i2c_send_start(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI16 frame_length) {
    (void)bus_id;
    if (i2c_write_start(I2C1, dev_addr, speed, frame_length) != 0) {
        return STSE_PLATFORM_BUS_ACK_ERROR;
    }
    return STSE_OK;
}

stse_return_code_t stse_platform_i2c_send_continue(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    (void)bus_id;
    (void)dev_addr;
    (void)speed;

    if (data_size != 0) {
        if (i2c_write_continue(I2C1, p_data, data_size) != 0) {
            return STSE_PLATFORM_BUS_ACK_ERROR;
        }
    }
    return STSE_OK;
}

stse_return_code_t stse_platform_i2c_send_stop(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    (void)bus_id;
    (void)dev_addr;
    (void)speed;

    if (i2c_write_stop(I2C1, p_data, data_size) != 0) {
        return STSE_PLATFORM_BUS_ACK_ERROR;
    }
    return STSE_OK;
}

stse_return_code_t stse_platform_i2c_receive_start(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI16 frame_length) {
    (void)bus_id;
    if (i2c_read_start(I2C1, dev_addr, speed, frame_length) != 0) {
        return STSE_PLATFORM_BUS_ACK_ERROR;
    }
    return STSE_OK;
}

stse_return_code_t stse_platform_i2c_receive_continue(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    (void)bus_id;
    (void)dev_addr;
    (void)speed;

    if (data_size != 0) {
        if (i2c_read_continue(I2C1, p_data, data_size) != 0) {
            return STSE_PLATFORM_BUS_ACK_ERROR;
        }
    }
    return STSE_OK;
}

stse_return_code_t stse_platform_i2c_receive_stop(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    (void)bus_id;
    (void)dev_addr;
    (void)speed;

    if (i2c_read_stop(I2C1, p_data, data_size) != 0) {
        return STSE_PLATFORM_BUS_ACK_ERROR;
    }
    return STSE_OK;
}
