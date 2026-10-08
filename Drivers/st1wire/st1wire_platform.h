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
#ifndef ST1WIRE_PLATFORM_H_

/* ---------- Static Platform Abstraction layer Declarations ---------- */

#include "st1wire.h"
#include "stm32l4xx.h"

#ifdef USE_FREERTOS
#define ST1WIRE_START_CRITICAL_SECTION \
    vTaskSuspendAll();                 \
    __disable_irq();
#define ST1WIRE_END_CRITICAL_SECTION \
    xTaskResumeAll();                \
    __enable_irq();
#else
#define ST1WIRE_START_CRITICAL_SECTION __disable_irq();
#define ST1WIRE_END_CRITICAL_SECTION __enable_irq();
#endif /* USE_FREERTOS */

st1wire_ReturnCode_t st1wire_platform_init(void);
st1wire_ReturnCode_t st1wire_platform_deinit(void);
void st1wire_platform_io_set(uint8_t bus_addr);
void st1wire_platform_io_clear(uint8_t bus_addr);
uint8_t st1wire_platform_io_get(uint8_t bus_addr);
void st1wire_platform_io_in(uint8_t bus_addr);
void st1wire_platform_io_out(uint8_t bus_addr);
void st1wire_platform_wake(uint8_t bus_addr);
void st1wire_platform_delay(uint32_t delay);
uint32_t st1wire_platform_get_cycle_count(void);
uint32_t st1wire_platform_get_cycles_per_us(void);
void st1wire_platform_start_timeout(uint32_t timeout);
int8_t st1wire_platform_is_timeout_exceeded(void);

#endif /*ST1WIRE_PLATFORM_H_*/
