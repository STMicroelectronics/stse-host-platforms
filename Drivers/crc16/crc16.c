/******************************************************************************
 * \file	crc16.c
 * \brief   CRC16 driver for STM32L452
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

#include "Drivers/crc16/crc16.h"

#ifdef CRC16_HW_IMP

typedef struct {
    uint32_t cr;
    uint32_t pol;
    uint32_t init;
} hw_crc_backup_t;

static inline void hw_crc_save_global_state(hw_crc_backup_t *backup) {
    backup->cr = CRC->CR;
    backup->pol = CRC->POL;
    backup->init = CRC->INIT;
}

// It's important to restore global state in case other libraries or parts of the code are also using the hardware CRC unit for different purposes.
// For instance with STM32 Crypto library, that expects 32 bit CRC configuration.
static inline void hw_crc_restore_global_state(const hw_crc_backup_t *backup) {
    CRC->INIT = backup->init;
    CRC->POL = backup->pol;
    CRC->CR = backup->cr;
}

static inline void hw_crc_configure_16b(uint16_t seed_value) {
    CRC->CR = (1UL << CRC_CR_POLYSIZE_Pos) | ((uint32_t)CRC16_REV_IN << CRC_CR_REV_IN_Pos) | ((uint32_t)CRC16_REV_OUT << CRC_CR_REV_OUT_Pos);

    CRC->POL = CRC16_POLY;
    CRC->INIT = seed_value;
    CRC->CR |= CRC_CR_RESET;
}

static inline uint16_t hw_crc_extract_and_update_context(crc16_context_t *ctx) {
    /* Temporarily disable REV_OUT to extract the raw running state */
    CRC->CR &= ~(1UL << CRC_CR_REV_OUT_Pos);
    ctx->crc_val = (uint16_t)CRC->DR;

    CRC->CR |= (1UL << CRC_CR_REV_OUT_Pos);

    return ~((uint16_t)CRC->DR);
}

void crc16_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
}

void crc16_InitContext(crc16_context_t *ctx) {
    ctx->crc_val = CRC_INITVALUE;
}

uint16_t crc16_Calculate(uint8_t *address, uint16_t length, crc16_context_t *ctx) {
    crc16_InitContext(ctx);
    return crc16_Accumulate(address, length, ctx);
}

uint16_t crc16_Accumulate(uint8_t *address, uint16_t length, crc16_context_t *ctx) {
    hw_crc_backup_t global_hw_state;
    uint16_t final_result;
    volatile uint8_t *p8 = (uint8_t *)&CRC->DR;

    hw_crc_save_global_state(&global_hw_state);

    hw_crc_configure_16b(ctx->crc_val);

    for (uint16_t i = 0; i < length; i++) {
        *p8 = address[i];
    }

    final_result = hw_crc_extract_and_update_context(ctx);

    hw_crc_restore_global_state(&global_hw_state);

    return final_result;
}

/* CRC16_HW_IMP */

#else

/* ---------------------- SW CRC16 Implementation --------------------- */

static const uint16_t crc16_tab[] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78};

void crc16_Init(void) {
    /* Nothing to initialise for SW implementation */
}

void crc16_InitContext(crc16_context_t *ctx) {
    ctx->crc_val = CRC_INITVALUE; /* 0xFFFF, pre-inversion running state */
}

uint16_t crc16_Calculate(uint8_t *address, uint16_t length, crc16_context_t *ctx) {
    crc16_InitContext(ctx);
    return crc16_Accumulate(address, length, ctx);
}

uint16_t crc16_Accumulate(uint8_t *address, uint16_t length, crc16_context_t *ctx) {
    for (uint16_t i = 0; i < length; i++) {
        ctx->crc_val = (ctx->crc_val >> 8) ^ crc16_tab[(ctx->crc_val ^ address[i]) & 0x00FF];
    }
    return ~ctx->crc_val;
}

#endif
