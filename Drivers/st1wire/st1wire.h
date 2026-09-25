/**
 ******************************************************************************
 * \file    st1wire.h
 * \brief	st1wie bit banging driver (header)
 * \author  STMicroelectronics - SMD application team
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
#ifndef ST1WIRE_H_
#define ST1WIRE_H_

#include "stm32l4xx.h"

/******************************* TIMINGS DEFINITIONS ***************************************/

/* - ST1Wire timings configuration (in us) */
#define ST1WIRE_IDLE 100
#define ST1WIRE_RECEIVE_TIMEOUT 34464

/* ST1Wire configuration  */
#define ST1WIRE_LONG_PULSE 5
#define ST1WIRE_SHORT_PULSE 2
#define ST1WIRE_ACK_PULSE 2
#define ST1WIRE_START_PULSE 10 * (ST1WIRE_LONG_PULSE + ST1WIRE_SHORT_PULSE)
#define ST1WIRE_INTER_BYTE_DELAY 10

/*********************** Exported functions ***************************************/

/** \defgroup st1wire ST1Wire Layer
 *  \brief ST1Wire bitbanging interface
 *  @{
*/

typedef enum {
    ST1WIRE_OK = 0x00,
    ST1WIRE_BUS_ARBITRATION_FAULT,
    ST1WIRE_BUS_ACK_ERROR,
    ST1WIRE_BUS_RECEIVE_TIMEOUT
} st1wire_ReturnCode_t;

/*!
 * \brief	Initialize ST1Wire bus
 * \result  ST1WIRE_OK on success ; st1wire_ReturnCode_t error code otherwise
 */
extern st1wire_ReturnCode_t st1wire_init(uint8_t busID);

/*!
 * \brief	De-initialise ST1Wire bus
 * \result  ST1WIRE_OK on success ; st1wire_ReturnCode_t error code otherwise
 */
extern st1wire_ReturnCode_t st1wire_deinit(uint8_t busID);

/*!
 * \brief					Send frame on ST1Wire bus
 * \param[in] bus_addr		Index of the ST1Wire bus
 * \param[in] speed			Communication speed (0 : slow	1: fast)
 * \param[in] *frame		Pointer to the applicative transmit buffer
 * \parame[in] frame_length	Length of the Frame to be sent
 */
extern st1wire_ReturnCode_t st1wire_SendFrame(uint8_t bus_addr,
                                              uint8_t dev_addr,
                                              uint8_t speed,
                                              uint8_t *frame,
                                              uint16_t frame_length);

/*!
 * \brief					Receive frame on ST1Wire bus
 * \param[in] bus_addr		Index of the ST1Wire bus
 * \param[in] speed			Communication speed (0 : slow	1: fast)
 * \param[in] *frame		Pointer to the applicative receive buffer
 * \parame[in] frame_length	Pointer to the applicative receive frame length variable
 */
extern st1wire_ReturnCode_t st1wire_ReceiveFrame(uint8_t bus_addr,
                                                 uint8_t dev_addr,
                                                 uint8_t speed,
                                                 uint8_t *frame,
                                                 uint16_t *pframe_length);

/*!
 * \brief					Wake ST1Wire device
 * \param[in] bus_addr		Index of the ST1Wire bus
 */
extern void st1wire_wake(uint8_t bus_addr);

/*!
 * \brief					Recover ST1Wire device
 * \param[in] bus_addr		Index of the ST1Wire bus
 * \param[in] speed			Communication speed (0 : slow	1: fast)
 */
extern void st1wire_recovery(uint8_t bus_addr,
                             uint8_t speed);

/*! @}*/

#endif /* ST1WIRE_H_ */
