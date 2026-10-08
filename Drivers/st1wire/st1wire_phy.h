/**
 ******************************************************************************
 * \file    st1wire_phy.h
 * \brief   ST1Wire physical layer (3C)
 ******************************************************************************
 */

#ifndef ST1WIRE_PHY_H_
#define ST1WIRE_PHY_H_

#include "st1wire.h"

void st1wire_phy_init(void);
void st1wire_phy_deinit(void);

st1wire_ReturnCode_t st1wire_phy_send_start(void);
st1wire_ReturnCode_t st1wire_phy_send_byte(uint8_t byte);
st1wire_ReturnCode_t st1wire_phy_receive_byte(uint8_t *byte);

void st1wire_phy_inter_byte_delay(void);
void st1wire_phy_wake(void);

#endif /* ST1WIRE_PHY_H_ */
