/**
 ******************************************************************************
 * \brief  ST1Wire Platform Hardware abstraction layer for G0C1RE_NUCLEO
 * \author STMicroelectronics SMD Application Team
 *****************************************************************************/

#include "st1wire_platform.h"
#include "Drivers/delay_ms/delay_ms.h"
#include "Drivers/delay_us/delay_us.h"
#include "stm32l4xx.h"


extern uint32_t SystemCoreClock;
volatile uint32_t st1wire_ref_cpu_cycles = 0;

/* ---------- Static Platform Abstraction layer Declarations ---------- */

st1wire_ReturnCode_t st1wire_platform_init(void) {
    /* - Initialize PA9 (ST1Wire line) as open-drain output */
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD9_Msk);
    GPIOA->OTYPER |= 1 << 9;
    GPIOA->ODR |= 1 << 9;
    GPIOA->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED9_Pos);
    GPIOA->MODER &= ~(GPIO_MODER_MODE9_Msk);
    GPIOA->MODER |= (0b01 << GPIO_MODER_MODE9_Pos);

    GPIOB->ODR &= ~(1 << GPIO_ODR_OD0_Pos);

    delay_us_init();
    delay_ms_init();

    /* Use the Cortex-M cycle counter for timing-sensitive ST1Wire sampling. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    return ST1WIRE_OK;
}

st1wire_ReturnCode_t st1wire_platform_deinit(void) {
    /* Do Nothing */
	return ST1WIRE_OK;
}

void st1wire_platform_io_set(uint8_t bus_addr) {
    /* SET IO bit-field in Output data register */
    GPIOA->ODR |= (1 << 9);
}

void st1wire_platform_io_clear(uint8_t bus_addr) {
    /* Clear IO bit-field in Output data register */
    GPIOA->ODR &= ~(1 << 9);
}

uint8_t st1wire_platform_io_get(uint8_t bus_addr) {
    /* Return PA9 Status from Input Data register */
    if ((GPIOA->IDR & 1 << 9) != 0) {
        return 1;
    } else {
        return 0;
    }
}

void st1wire_platform_io_in(uint8_t bus_addr) {
    /* Set PA9 as input */
    GPIOA->MODER &= ~(GPIO_MODER_MODE9_Msk);
}

void st1wire_platform_io_out(uint8_t bus_addr) {
    /* Set PA9 as output */
    GPIOA->MODER |= (0b01 << GPIO_MODER_MODE9_Pos);
}

void st1wire_platform_delay(uint32_t delay) {
    delay_us(delay);
}

uint32_t st1wire_platform_get_cycle_count(void) {
    return DWT->CYCCNT;
}

uint32_t st1wire_platform_get_cycles_per_us(void) {
    return SystemCoreClock / 1000000U;
}

void st1wire_platform_wake(uint8_t bus_addr) {
    st1wire_platform_io_clear(bus_addr);
    st1wire_platform_delay(1000);
    st1wire_platform_io_set(bus_addr);
    st1wire_platform_delay(8000);
}

void st1wire_platform_start_timeout(uint32_t timeout) {
    timeout_us_start(timeout);
}

int8_t st1wire_platform_is_timeout_exceeded(void) {
    return timeout_us_get_status();
}
