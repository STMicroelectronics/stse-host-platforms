/**
 ******************************************************************************
 * \file    st1wire_platform.h
 * \brief   ST1Wire hardware abstraction for NUCLEO-L452RE (PA9, TIM1, DMA1)
 ******************************************************************************
 */

#ifndef ST1WIRE_PLATFORM_H_
#define ST1WIRE_PLATFORM_H_

#include <stdint.h>

void st1wire_platform_init(void);
void st1wire_platform_deinit(void);

/* Line driven as a GPIO (start condition, wake-up, idle detection) */
void st1wire_platform_io_in(void);
void st1wire_platform_io_out(void);
void st1wire_platform_io_set(void);
void st1wire_platform_io_clear(void);
uint8_t st1wire_platform_io_get(void);

/*
 * Line driven by the timer: the line toggles at each time in toggles[]
 * (first toggle pulls it low) while the time of every edge seen on the line
 * is stored in edges[]. Times are in microseconds from the start.
 */
void st1wire_platform_waveform_start(const uint16_t *toggles, uint16_t toggle_count,
                                     volatile uint16_t *edges, uint16_t edge_count);
void st1wire_platform_waveform_stop(void);
uint16_t st1wire_platform_edges_remaining(void);
uint16_t st1wire_platform_waveform_time(void);

/* Microsecond time base */
void st1wire_platform_delay_us(uint16_t us);
void st1wire_platform_timeout_start(uint16_t us);
uint8_t st1wire_platform_timeout_expired(void);

#endif /* ST1WIRE_PLATFORM_H_ */
