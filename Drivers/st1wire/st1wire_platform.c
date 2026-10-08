/**
 ******************************************************************************
 * \file    st1wire_platform.c
 * \brief   ST1Wire hardware abstraction for NUCLEO-L452RE (PA9, TIM1, DMA1)
 ******************************************************************************
 *
 * The ST1Wire line is PA9, open-drain, which is also TIM1_CH2 (AF1).
 *
 * Outside of a byte transfer the pin is a plain GPIO. During a byte it is
 * handed over to TIM1, clocked at 1 MHz:
 *  - CH2 is in output compare toggle mode. DMA1 channel 3 loads the next
 *    toggle time into CCR2 after each match, so the host waveform is generated
 *    without the CPU.
 *  - CH1 captures both edges of TI2, i.e. the same pin, and DMA1 channel 2
 *    stores the timestamps. This records the edges driven by the device as
 *    well as the host's own.
 * Interrupts can therefore stay enabled during a transfer.
 */

#include "st1wire_platform.h"
#include "Drivers/delay_us/delay_us.h"
#include "stm32l4xx.h"

#define ST1WIRE_TIM_FREQUENCY_HZ 1000000UL
#define ST1WIRE_DMA_REQ_TIM1 7UL

static uint32_t tim1_clock_hz(void) {
    uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos;

    /* APB2 timers run at twice PCLK2 when APB2 is divided */
    if (ppre2 < 4U) {
        return SystemCoreClock;
    }
    return (SystemCoreClock >> (ppre2 - 3U)) * 2UL;
}

static void tim1_stop(void) {
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM1->DIER = 0U;
    TIM1->CCER = 0U;
    TIM1->BDTR &= ~TIM_BDTR_MOE;
    TIM1->SR = 0U;
}

static void tim1_setup(void) {
    tim1_stop();

    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE9_Msk) | GPIO_MODER_MODE9_1;

    TIM1->PSC = (tim1_clock_hz() / ST1WIRE_TIM_FREQUENCY_HZ) - 1UL;
    TIM1->ARR = 0xFFFFU;
    TIM1->CNT = 0U;
    TIM1->CR1 = 0U;
    TIM1->CR2 = 0U;
    TIM1->SMCR = 0U;

    /* CH1: input capture on TI2, both edges. CH2: toggle on match, active low */
    TIM1->CCMR1 = (2UL << TIM_CCMR1_CC1S_Pos) |
                  (3UL << TIM_CCMR1_OC2M_Pos);
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NP |
                 TIM_CCER_CC2E | TIM_CCER_CC2P;
    TIM1->BDTR |= TIM_BDTR_MOE;

    TIM1->EGR = TIM_EGR_UG;
    TIM1->SR = 0U;
}

static void dma_capture_setup(volatile uint16_t *edges, uint16_t count) {
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = DMA_IFCR_CGIF2;

    DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~DMA_CSELR_C2S_Msk) |
                        (ST1WIRE_DMA_REQ_TIM1 << DMA_CSELR_C2S_Pos);

    DMA1_Channel2->CPAR = (uint32_t)&TIM1->CCR1;
    DMA1_Channel2->CMAR = (uint32_t)edges;
    DMA1_Channel2->CNDTR = count;
    DMA1_Channel2->CCR = DMA_CCR_MINC | DMA_CCR_PSIZE_0 | DMA_CCR_MSIZE_0 |
                         DMA_CCR_PL_1;
}

static void dma_toggle_setup(const uint16_t *toggles, uint16_t count) {
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = DMA_IFCR_CGIF3;

    DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~DMA_CSELR_C3S_Msk) |
                        (ST1WIRE_DMA_REQ_TIM1 << DMA_CSELR_C3S_Pos);

    DMA1_Channel3->CPAR = (uint32_t)&TIM1->CCR2;
    DMA1_Channel3->CMAR = (uint32_t)toggles;
    DMA1_Channel3->CNDTR = count;
    DMA1_Channel3->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_PSIZE_0 |
                         DMA_CCR_MSIZE_0 | DMA_CCR_PL_1 | DMA_CCR_PL_0;
}

void st1wire_platform_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    st1wire_platform_waveform_stop();

    GPIOA->OTYPER |= GPIO_OTYPER_OT9;
    GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED9_Msk;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD9_Msk;
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL9_Msk) |
                    (1UL << GPIO_AFRH_AFSEL9_Pos);

    st1wire_platform_io_set();
    st1wire_platform_io_out();

    delay_us_init();
}

void st1wire_platform_deinit(void) {
    st1wire_platform_waveform_stop();
}

void st1wire_platform_io_in(void) {
    GPIOA->MODER &= ~GPIO_MODER_MODE9_Msk;
}

void st1wire_platform_io_out(void) {
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE9_Msk) | GPIO_MODER_MODE9_0;
}

void st1wire_platform_io_set(void) {
    GPIOA->BSRR = GPIO_BSRR_BS9;
}

void st1wire_platform_io_clear(void) {
    GPIOA->BRR = GPIO_BRR_BR9;
}

uint8_t st1wire_platform_io_get(void) {
    return ((GPIOA->IDR & GPIO_IDR_ID9) != 0U) ? 1U : 0U;
}

void st1wire_platform_waveform_start(const uint16_t *toggles, uint16_t toggle_count,
                                     volatile uint16_t *edges, uint16_t edge_count) {
    tim1_setup();

    dma_capture_setup(edges, edge_count);
    dma_toggle_setup(&toggles[1], (uint16_t)(toggle_count - 1U));

    TIM1->CCR2 = toggles[0];
    TIM1->DIER = TIM_DIER_CC1DE | TIM_DIER_CC2DE;

    DMA1_Channel2->CCR |= DMA_CCR_EN;
    DMA1_Channel3->CCR |= DMA_CCR_EN;

    TIM1->CNT = 0U;
    TIM1->SR = 0U;
    TIM1->CR1 |= TIM_CR1_CEN;
}

void st1wire_platform_waveform_stop(void) {
    tim1_stop();

    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = DMA_IFCR_CGIF2 | DMA_IFCR_CGIF3;

    st1wire_platform_io_in();
}

uint16_t st1wire_platform_edges_remaining(void) {
    return (uint16_t)DMA1_Channel2->CNDTR;
}

uint16_t st1wire_platform_waveform_time(void) {
    return (uint16_t)TIM1->CNT;
}

void st1wire_platform_delay_us(uint16_t us) {
    delay_us(us);
}

void st1wire_platform_timeout_start(uint16_t us) {
    timeout_us_start(us);
}

uint8_t st1wire_platform_timeout_expired(void) {
    return timeout_us_get_status();
}
