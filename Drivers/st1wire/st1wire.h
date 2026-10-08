/**
 ******************************************************************************
 * \file    st1wire.h
 * \brief   ST1Wire protocol driver (3C)
 ******************************************************************************
 */

#ifndef ST1WIRE_H_
#define ST1WIRE_H_

#include <stdint.h>

typedef enum {
    ST1WIRE_OK = 0x00,
    ST1WIRE_BUS_ARBITRATION_FAULT,
    ST1WIRE_BUS_ACK_ERROR,
    ST1WIRE_BUS_RECEIVE_TIMEOUT,
    ST1WIRE_FRAME_TOO_LONG
} st1wire_ReturnCode_t;

st1wire_ReturnCode_t st1wire_init(void);
st1wire_ReturnCode_t st1wire_deinit(void);

/*!
 * \brief Wake the device up from hibernate
 */
void st1wire_wake(void);

/*!
 * \brief           Send a frame to the device
 * \param[in] dev_addr  Device address, 0 to omit the address byte
 * \param[in] frame     Frame payload
 * \param[in] length    Payload length (11 bits)
 */
st1wire_ReturnCode_t st1wire_SendFrame(uint8_t dev_addr, const uint8_t *frame, uint16_t length);

/*!
 * \brief           Read the device response frame
 * \param[in]  dev_addr     Device address, 0 to omit the address byte
 * \param[out] frame        Response payload
 * \param[in]  max_length   Size of the frame buffer
 * \param[out] length       Response payload length, as announced by the device
 * \return ST1WIRE_FRAME_TOO_LONG if the response does not fit in frame
 */
st1wire_ReturnCode_t st1wire_ReceiveFrame(uint8_t dev_addr, uint8_t *frame, uint16_t max_length, uint16_t *length);

#endif /* ST1WIRE_H_ */
