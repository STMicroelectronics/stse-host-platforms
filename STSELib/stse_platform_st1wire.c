/******************************************************************************
 * \file	stse_platform_st1wire.c
 * \brief   STSecureElement ST1Wire platform (source)
 * \author  STMicroelectronics - CS application team
 *
 ******************************************************************************
 * \attention
 *
 * <h2><center>&copy; COPYRIGHT 2022 STMicroelectronics</center></h2>
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "core/stse_platform.h"
#include "drivers/st1wire/st1wire.h"
#include <stdlib.h>

#ifdef STSE_CONF_USE_ST1WIRE

#define STSE_PLATFORM_ST1WIRE_BUFFER_LENGTH 752U
static PLAT_UI8 st1wire_buffer[STSE_PLATFORM_ST1WIRE_BUFFER_LENGTH];
static PLAT_UI16 st1wire_frame_size;
static volatile PLAT_UI16 st1wire_frame_offset;

stse_return_code_t stse_platform_st1wire_init(PLAT_UI8 bus_id, void *pArg) {
    st1wire_ReturnCode_t ret;
    (void)bus_id;
    (void)pArg;

    ret = st1wire_init(bus_id);
    if (ret == ST1WIRE_OK) {
        return STSE_OK;
    } else {
        return STSE_PLATFORM_SERVICES_INIT_ERROR;
    }
}

stse_return_code_t stse_platform_st1wire_wake(PLAT_UI8 bus_id,
                                             PLAT_UI8 dev_addr,
                                             PLAT_UI16 speed) {
    (void)dev_addr;
    (void)speed;

    st1wire_wake(bus_id);

    return (STSE_OK);
}

stse_return_code_t stse_platform_st1wire_send_start(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI16 frame_length) {
    (void)bus_id;
    (void)dev_addr;
    (void)speed;

    /* - Check buffer overflow */
    if (frame_length > sizeof(st1wire_buffer) / sizeof(st1wire_buffer[0])) {
        return STSE_PLATFORM_BUFFER_ERR;
    }

    st1wire_frame_size = frame_length;
    st1wire_frame_offset = 0;

    return STSE_OK;
}

stse_return_code_t stse_platform_st1wire_send_continue(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    (void)bus_id;
    (void)dev_addr;
    (void)speed;

    if (data_size != 0) {
        if (p_data == NULL) {
            memset((st1wire_buffer + st1wire_frame_offset), 0x00, data_size);
        } else {
            memcpy((st1wire_buffer + st1wire_frame_offset), p_data, data_size);
        }
        st1wire_frame_offset += data_size;
    }

    return STSE_OK;
}

stse_return_code_t stse_platform_st1wire_send_stop(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    stse_return_code_t ret;

    ret = stse_platform_st1wire_send_continue(
        bus_id,
        dev_addr,
        speed,
        p_data,
        data_size);

    /* - Send ST1Wire frame buffer */
    if (ret == STSE_OK) {
        ret = (stse_return_code_t)st1wire_SendFrame(
            bus_id,
            dev_addr,
            speed,
            st1wire_buffer,
            st1wire_frame_size);
    }

    if (ret != STSE_OK) {
        ret = STSE_PLATFORM_BUS_ACK_ERROR;
    }

    return ret;
}

stse_return_code_t stse_platform_st1wire_receive_start(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI16 frame_length) {
    PLAT_I8 ret = 0;

    /* Check read buffer overflow */
    if (frame_length > STSE_PLATFORM_ST1WIRE_BUFFER_LENGTH) {
        return STSE_PLATFORM_BUFFER_ERR;
    }

    /* - Read full Frame */
    ret = st1wire_ReceiveFrame(
        bus_id,
        dev_addr,
        speed,
        st1wire_buffer,
        &st1wire_frame_size);

    if (ret != 0) {
        return STSE_PLATFORM_BUS_ACK_ERROR;
    }

    /* - Reset read offset */
    st1wire_frame_offset = 0;

    return STSE_OK;
}

stse_return_code_t stse_platform_st1wire_receive_continue(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    (void)bus_id;
    (void)dev_addr;
    (void)speed;

    if (p_data != NULL) {

        /* Check read overflow */
        if ((st1wire_frame_size - st1wire_frame_offset) < data_size) {
            return STSE_PLATFORM_BUFFER_ERR;
        }

        /* Copy buffer content */
        memcpy(p_data, (st1wire_buffer + st1wire_frame_offset), data_size);
    }

    st1wire_frame_offset += data_size;

    return STSE_OK;
}

stse_return_code_t stse_platform_st1wire_receive_stop(
    PLAT_UI8 bus_id,
    PLAT_UI8 dev_addr,
    PLAT_UI16 speed,
    PLAT_UI8 *p_data,
    PLAT_UI16 data_size) {
    /*- Copy last element*/
    stse_platform_st1wire_receive_continue(bus_id, dev_addr, speed, p_data, data_size);

    st1wire_frame_offset = 0;

    return (STSE_OK);
}
#endif
