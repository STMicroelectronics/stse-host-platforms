/**
 ******************************************************************************
 * \file    st1wire.c
 * \brief   ST1Wire protocol driver (3C)
 ******************************************************************************
 */

#include "st1wire.h"
#include "st1wire_phy.h"

#define ST1WIRE_STATUS_OK 0x20U

static st1wire_ReturnCode_t send_next_byte(uint8_t byte) {
    st1wire_phy_inter_byte_delay();
    return st1wire_phy_send_byte(byte);
}

static st1wire_ReturnCode_t receive_next_byte(uint8_t *byte) {
    st1wire_phy_inter_byte_delay();
    return st1wire_phy_receive_byte(byte);
}

/* Start condition, optional device address, then the frame length MSB first */
static st1wire_ReturnCode_t send_header(uint8_t dev_addr, uint16_t length) {
    st1wire_ReturnCode_t ret;

    ret = st1wire_phy_send_start();
    if (ret != ST1WIRE_OK) {
        return ret;
    }

    if (dev_addr != 0U) {
        ret = st1wire_phy_send_byte(dev_addr);
        if (ret != ST1WIRE_OK) {
            return ret;
        }
        st1wire_phy_inter_byte_delay();
    }

    ret = st1wire_phy_send_byte((uint8_t)((length >> 8U) & 0x07U));
    if (ret == ST1WIRE_OK) {
        ret = send_next_byte((uint8_t)(length & 0xFFU));
    }

    return ret;
}

st1wire_ReturnCode_t st1wire_init(void) {
    st1wire_phy_init();
    return ST1WIRE_OK;
}

st1wire_ReturnCode_t st1wire_deinit(void) {
    st1wire_phy_deinit();
    return ST1WIRE_OK;
}

void st1wire_wake(void) {
    st1wire_phy_wake();
}

st1wire_ReturnCode_t st1wire_SendFrame(uint8_t dev_addr, const uint8_t *frame, uint16_t length) {
    st1wire_ReturnCode_t ret;
    uint8_t status;
    uint16_t i;

    ret = send_header(dev_addr, length);

    for (i = 0U; (ret == ST1WIRE_OK) && (i < length); i++) {
        ret = send_next_byte(frame[i]);
    }

    if (ret == ST1WIRE_OK) {
        ret = receive_next_byte(&status);
        if ((ret == ST1WIRE_OK) && (status != ST1WIRE_STATUS_OK)) {
            ret = ST1WIRE_BUS_ACK_ERROR;
        }
    }

    st1wire_phy_inter_byte_delay();

    return ret;
}

st1wire_ReturnCode_t st1wire_ReceiveFrame(uint8_t dev_addr, uint8_t *frame, uint16_t max_length, uint16_t *length) {
    st1wire_ReturnCode_t ret;
    uint8_t status;
    uint8_t length_msb;
    uint8_t length_lsb;
    uint16_t i;

    /* An empty frame asks the device for its response */
    ret = send_header(dev_addr, 0U);
    if (ret != ST1WIRE_OK) {
        return ret;
    }

    ret = receive_next_byte(&status);
    if ((ret != ST1WIRE_OK) || (status != ST1WIRE_STATUS_OK)) {
        return ST1WIRE_BUS_ACK_ERROR;
    }

    ret = receive_next_byte(&length_msb);
    if (ret == ST1WIRE_OK) {
        ret = receive_next_byte(&length_lsb);
    }

    if (ret == ST1WIRE_OK) {
        *length = (uint16_t)(((uint16_t)length_msb << 8U) | length_lsb);
        if (*length > max_length) {
            ret = ST1WIRE_FRAME_TOO_LONG;
        }
    }

    for (i = 0U; (ret == ST1WIRE_OK) && (i < *length); i++) {
        ret = receive_next_byte(&frame[i]);
    }

    st1wire_phy_inter_byte_delay();

    return ret;
}
