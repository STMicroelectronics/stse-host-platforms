/******************************************************************************
 * \file	i2c.c
 * \brief   I2C driver for STM32L452
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

#include <stddef.h>
#include "Drivers/i2c/i2c.h"
#include "Drivers/delay_ms/delay_ms.h"

static uint16_t i2c_speed = 100;

void i2c_deinit(I2C_TypeDef *pI2C) {
    // Do nothing
    (void)pI2C;
}

uint8_t i2c_init(I2C_TypeDef *pI2C) {
    /* - Clear PE bit */
    pI2C->CR1 &= ~(I2C_CR1_PE);

    /* - Configure ANFOFF, DFN & Clock stretching */
    pI2C->CR1 |= (0b1 << I2C_CR1_ANFOFF_Pos) |   // Analog  Noise Filtering disabled
                 (0x0 << I2C_CR1_DNF_Pos) |      // Digital Noise Filtering disabled
                 (0b0 << I2C_CR1_NOSTRETCH_Pos); // Clock stretching enabled

    if (i2c_speed == 400) {
        /* - Set I2C1 Timings for 400kHz (Fast mode) */
        pI2C->TIMINGR = (0x01 << I2C_TIMINGR_PRESC_Pos) |
                        (0x26 << I2C_TIMINGR_SCLL_Pos) |
                        (0x1D << I2C_TIMINGR_SCLH_Pos) |
                        (0x01 << I2C_TIMINGR_SDADEL_Pos) |
                        (0x0A << I2C_TIMINGR_SCLDEL_Pos);
    } else {
        /* - Set I2C1 Timings for 100kHz (Standard mode) */
        pI2C->TIMINGR = (0x03 << I2C_TIMINGR_PRESC_Pos) |
                        (0x50 << I2C_TIMINGR_SCLL_Pos) |
                        (0x47 << I2C_TIMINGR_SCLH_Pos) |
                        (0x01 << I2C_TIMINGR_SDADEL_Pos) |
                        (0x09 << I2C_TIMINGR_SCLDEL_Pos);
    }

    /* - Enable pI2C */
    pI2C->CR1 |= I2C_CR1_PE;

    return 0;
}

int8_t i2c_write(I2C_TypeDef *pI2C, uint8_t slave_address, uint16_t speed, uint8_t *pbuffer, uint16_t size) {
    uint16_t i = 0;
    uint16_t offset = 0;

    uint16_t xfer_length = size;
    uint8_t xfer_size;

    i2c_speed = speed;
    i2c_init(pI2C);

    if (xfer_length > 0xFF) {
        xfer_size = 0xFF;
    } else {
        xfer_size = xfer_length;
    }

    /* - Xfer Configuration  */
    pI2C->CR2 = (0x00 << I2C_CR2_ADD10_Pos) |
                (0x00 << I2C_CR2_RD_WRN_Pos) |
                (xfer_size << I2C_CR2_NBYTES_Pos) |
                (0x01 << I2C_CR2_AUTOEND_Pos) |
                (slave_address << (I2C_CR2_SADD_Pos + 1));
    if (xfer_length > 0xFF) {
        pI2C->CR2 |= I2C_CR2_RELOAD;
    }

    /* - Start Xfer */
    while (pI2C->ISR & I2C_ISR_BUSY) {
        /* Clear any stray STOPF or NACKF from wake pulse */
        if (pI2C->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF)) {
            pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        }
    }
    pI2C->CR2 |= I2C_CR2_START;

    while (xfer_length > 0) {
        /* - Send data */
        for (i = 0; i < xfer_size; i++) {
            /* - Wait for previous data to be sent */
            while (!(pI2C->ISR & I2C_ISR_TXE)) {
                /* - Return error in case of NACK */
                if (pI2C->ISR & I2C_ISR_NACKF) {
                    pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
                    return -1;
                }
            }
            pI2C->TXDR = (uint8_t)*(pbuffer + (i + offset));
        }

        xfer_length -= xfer_size;
        if (xfer_length > 0) {
            while (!(pI2C->ISR & I2C_ISR_TCR))
                ;
            offset += 0xFF;
            pI2C->CR2 &= ~(I2C_CR2_NBYTES_Msk);
            if (xfer_length > 0xFF) {
                xfer_size = 0xFF;
                pI2C->CR2 |= (xfer_size << I2C_CR2_NBYTES_Pos);
                pI2C->CR2 |= I2C_CR2_RELOAD;
            } else {
                xfer_size = xfer_length;
                pI2C->CR2 |= (xfer_size << I2C_CR2_NBYTES_Pos);
                pI2C->CR2 &= ~I2C_CR2_RELOAD;
            }
        }
    }

    while ((pI2C->ISR & I2C_ISR_STOPF) == 0)
        ;

    return 0;
}

int8_t i2c_read(I2C_TypeDef *pI2C, uint8_t slave_address, uint16_t speed, uint8_t *pbuffer, uint16_t size) {
    uint32_t i = 0;
    uint16_t xfer_length;
    uint16_t xfer_size;

    (void)(speed);

    xfer_length = size;
    if (xfer_length > 0xFF) {
        xfer_size = 0xFF;
    } else {
        xfer_size = xfer_length;
    }

    /* - Xfer Configuration  */
    pI2C->CR2 = (0x00 << I2C_CR2_ADD10_Pos) |
                (0x01 << I2C_CR2_RD_WRN_Pos) |
                (xfer_size << I2C_CR2_NBYTES_Pos) |
                (0x01 << I2C_CR2_AUTOEND_Pos) |
                (slave_address << (I2C_CR2_SADD_Pos + 1));
    if (xfer_length > 0xFF) {
        pI2C->CR2 |= I2C_CR2_RELOAD;
    }

    /* - Start Xfer */
    while (pI2C->ISR & I2C_ISR_BUSY) {
        if (pI2C->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF)) {
            pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        }
    }
    pI2C->CR2 |= I2C_CR2_START;

    while (!(pI2C->ISR & I2C_ISR_RXNE)) {
        /*- Check if NACK */
        if (pI2C->ISR & I2C_ISR_NACKF) {
            pI2C->ICR |= I2C_ICR_NACKCF;
            return -1;
        }
    }

    while (xfer_length > 0) {
        /* - Read data */
        for (i = 0; i < xfer_size; i++) {
            /*- Wait for data reception */
            while (!(pI2C->ISR & I2C_ISR_RXNE))
                ;
            /*- Store data  */
            *(pbuffer++) = (uint8_t)pI2C->RXDR;
        }

        xfer_length -= xfer_size;
        if (xfer_length > 0) {
            while (!(pI2C->ISR & I2C_ISR_TCR))
                ;
            pI2C->CR2 &= ~(I2C_CR2_NBYTES_Msk);
            if (xfer_length > 0xFF) {
                xfer_size = 0xFF;
                pI2C->CR2 |= (xfer_size << I2C_CR2_NBYTES_Pos);
                pI2C->CR2 |= I2C_CR2_RELOAD;
            } else {
                xfer_size = xfer_length;
                pI2C->CR2 |= (xfer_size << I2C_CR2_NBYTES_Pos);
                pI2C->CR2 &= ~I2C_CR2_RELOAD;
            }
        }
    }

    while ((pI2C->ISR & I2C_ISR_STOPF) == 0)
        ;

    return 0;
}

void i2c_wake(I2C_TypeDef *pI2C, uint8_t slave_address) {

    /* - Xfer Configuration  */
    pI2C->CR2 = (0x00 << I2C_CR2_ADD10_Pos) |
                (0x00 << I2C_CR2_RD_WRN_Pos) |
                (0x00 << I2C_CR2_NBYTES_Pos) |
                (0x01 << I2C_CR2_AUTOEND_Pos) |
                (slave_address << (I2C_CR2_SADD_Pos + 1));
    /* - Start Xfer */
    pI2C->CR2 |= I2C_CR2_START;

    /* Wait for autoend stop flag or NACK to settle the wake pulse */
    while ((pI2C->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF)) == 0)
        ;
    pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
}

/* Zero-Copy Scatter-Gather Streaming Implementation ------------------------*/

static uint16_t i2c_tx_remaining = 0;
static uint8_t  i2c_tx_chunk_size = 0;
static uint16_t i2c_rx_remaining = 0;
static uint8_t  i2c_rx_chunk_size = 0;

int8_t i2c_write_start(I2C_TypeDef *pI2C, uint8_t slave_address, uint16_t speed, uint16_t total_length) {
    i2c_speed = speed;
    i2c_init(pI2C);

    i2c_tx_remaining = total_length;
    if (i2c_tx_remaining > 0xFF) {
        i2c_tx_chunk_size = 0xFF;
    } else {
        i2c_tx_chunk_size = (uint8_t)i2c_tx_remaining;
    }

    /* - Xfer Configuration */
    pI2C->CR2 = (0x00 << I2C_CR2_ADD10_Pos) |
                (0x00 << I2C_CR2_RD_WRN_Pos) |
                (i2c_tx_chunk_size << I2C_CR2_NBYTES_Pos) |
                (0x01 << I2C_CR2_AUTOEND_Pos) |
                (slave_address << (I2C_CR2_SADD_Pos + 1));
    if (i2c_tx_remaining > 0xFF) {
        pI2C->CR2 |= I2C_CR2_RELOAD;
    }

    /* - Start Xfer */
    while (pI2C->ISR & I2C_ISR_BUSY) {
        /* Clear any stray STOPF or NACKF from wake pulse */
        if (pI2C->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF)) {
            pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        }
    }
    pI2C->CR2 |= I2C_CR2_START;

    return 0;
}

int8_t i2c_write_continue(I2C_TypeDef *pI2C, const uint8_t *pdata, uint16_t length) {
    uint16_t i;
    uint8_t byte;

    for (i = 0; i < length; i++) {
        /* Wait for TXDR empty */
        while (!(pI2C->ISR & I2C_ISR_TXE)) {
            if (pI2C->ISR & I2C_ISR_NACKF) {
                pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
                return -1;
            }
        }

        byte = (pdata != NULL) ? pdata[i] : 0x00;
        pI2C->TXDR = byte;
        i2c_tx_remaining--;

        i2c_tx_chunk_size--;
        if (i2c_tx_chunk_size == 0 && i2c_tx_remaining > 0) {
            /* Chunk reached 0, wait for TCR before programming next chunk */
            while (!(pI2C->ISR & I2C_ISR_TCR)) {
                if (pI2C->ISR & I2C_ISR_NACKF) {
                    pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
                    return -1;
                }
            }
            pI2C->CR2 &= ~(I2C_CR2_NBYTES_Msk);
            if (i2c_tx_remaining > 0xFF) {
                i2c_tx_chunk_size = 0xFF;
                pI2C->CR2 |= (i2c_tx_chunk_size << I2C_CR2_NBYTES_Pos) | I2C_CR2_RELOAD;
            } else {
                i2c_tx_chunk_size = (uint8_t)i2c_tx_remaining;
                pI2C->CR2 |= (i2c_tx_chunk_size << I2C_CR2_NBYTES_Pos);
                pI2C->CR2 &= ~I2C_CR2_RELOAD;
            }
        }
    }

    return 0;
}

int8_t i2c_write_stop(I2C_TypeDef *pI2C, const uint8_t *pdata, uint16_t length) {
    int8_t ret = 0;
    if (length > 0) {
        ret = i2c_write_continue(pI2C, pdata, length);
        if (ret != 0) {
            return ret;
        }
    }

    /* Wait for autoend STOP flag */
    while ((pI2C->ISR & I2C_ISR_STOPF) == 0) {
        if (pI2C->ISR & I2C_ISR_NACKF) {
            pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
            return -1;
        }
    }
    pI2C->ICR = I2C_ICR_STOPCF;

    return 0;
}

int8_t i2c_read_start(I2C_TypeDef *pI2C, uint8_t slave_address, uint16_t speed, uint16_t total_length) {
    (void)speed;

    i2c_rx_remaining = total_length;
    if (i2c_rx_remaining > 0xFF) {
        i2c_rx_chunk_size = 0xFF;
    } else {
        i2c_rx_chunk_size = (uint8_t)i2c_rx_remaining;
    }

    /* - Xfer Configuration */
    pI2C->CR2 = (0x00 << I2C_CR2_ADD10_Pos) |
                (0x01 << I2C_CR2_RD_WRN_Pos) |
                (i2c_rx_chunk_size << I2C_CR2_NBYTES_Pos) |
                (0x01 << I2C_CR2_AUTOEND_Pos) |
                (slave_address << (I2C_CR2_SADD_Pos + 1));
    if (i2c_rx_remaining > 0xFF) {
        pI2C->CR2 |= I2C_CR2_RELOAD;
    }

    /* - Start Xfer */
    while (pI2C->ISR & I2C_ISR_BUSY) {
        if (pI2C->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF)) {
            pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        }
    }
    pI2C->CR2 |= I2C_CR2_START;

    /* Wait for START to be accepted: check for NACK */
    if (pI2C->ISR & I2C_ISR_NACKF) {
        pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        return -1;
    }

    return 0;
}

int8_t i2c_read_continue(I2C_TypeDef *pI2C, uint8_t *pdata, uint16_t length) {
    uint16_t i;
    uint8_t dummy;

    for (i = 0; i < length; i++) {
        while (!(pI2C->ISR & I2C_ISR_RXNE)) {
            if (pI2C->ISR & I2C_ISR_NACKF) {
                pI2C->ICR |= I2C_ICR_NACKCF;
                return -1;
            }
        }

        if (pdata != NULL) {
            pdata[i] = (uint8_t)pI2C->RXDR;
        } else {
            dummy = (uint8_t)pI2C->RXDR;
            (void)dummy;
        }
        i2c_rx_remaining--;

        i2c_rx_chunk_size--;
        if (i2c_rx_chunk_size == 0 && i2c_rx_remaining > 0) {
            /* Chunk reached 0, wait for TCR before programming next chunk */
            while (!(pI2C->ISR & I2C_ISR_TCR)) {
                if (pI2C->ISR & I2C_ISR_NACKF) {
                    pI2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
                    return -1;
                }
            }
            pI2C->CR2 &= ~(I2C_CR2_NBYTES_Msk);
            if (i2c_rx_remaining > 0xFF) {
                i2c_rx_chunk_size = 0xFF;
                pI2C->CR2 |= (i2c_rx_chunk_size << I2C_CR2_NBYTES_Pos) | I2C_CR2_RELOAD;
            } else {
                i2c_rx_chunk_size = (uint8_t)i2c_rx_remaining;
                pI2C->CR2 |= (i2c_rx_chunk_size << I2C_CR2_NBYTES_Pos);
                pI2C->CR2 &= ~I2C_CR2_RELOAD;
            }
        }
    }

    return 0;
}

int8_t i2c_read_stop(I2C_TypeDef *pI2C, uint8_t *pdata, uint16_t length) {
    int8_t ret = 0;
    if (length > 0) {
        ret = i2c_read_continue(pI2C, pdata, length);
        if (ret != 0) {
            return ret;
        }
    }

    /* Wait for autoend STOP flag */
    while ((pI2C->ISR & I2C_ISR_STOPF) == 0) {
        if (pI2C->ISR & I2C_ISR_NACKF) {
            pI2C->ICR |= I2C_ICR_NACKCF;
            return -1;
        }
    }
    pI2C->ICR = I2C_ICR_STOPCF;

    return 0;
}
