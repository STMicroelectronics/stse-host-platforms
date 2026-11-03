/******************************************************************************
 * \file	stse_platform_crc.c
 * \brief   STSecureElement CRC16 platform file
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
#include "stse_conf.h"
#include "stselib.h"

/*
 * One context per logical device is the intended pattern when bus_id
 * is used. Extend to an array indexed by bus_id if multi-device support
 * is required at this layer.
 */
static crc16_context_t crc_ctx;

stse_return_code_t stse_platform_crc16_init(void *p_arg) {
    (void)p_arg;

    crc16_Init();
    return STSE_OK;
}

PLAT_UI16 stse_platform_crc16_calculate(PLAT_UI8 *p_buffer, PLAT_UI16 length) {
    crc16_InitContext(&crc_ctx);
    return crc16_Calculate(p_buffer, length, &crc_ctx);
}

PLAT_UI16 stse_platform_crc16_accumulate(PLAT_UI8 *p_buffer, PLAT_UI16 length) {
    return crc16_Accumulate(p_buffer, length, &crc_ctx);
}
