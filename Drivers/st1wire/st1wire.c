/**
 ******************************************************************************
 * \file    st1wire.c
 * \brief	st1wie bit banging driver (sources)
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

/* Platform configuration parameters */
#include "st1wire.h"
#include "st1wire_platform.h"

/**
  ******************************************************************************
  * \file    st1wire.c
  * \brief st1wie bit banging driver (sources)
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

/* ---------- Static functions Definition ---------- */

static st1wire_ReturnCode_t st1wire_send_byte(uint8_t bus_addr, uint8_t byte);

static st1wire_ReturnCode_t st1wire_receive_byte(uint8_t bus_addr, uint8_t *rcv_byte);

static st1wire_ReturnCode_t st1wire_send_start(uint8_t bus_addr);

static uint32_t st1wire_elapsed_cycles(uint32_t start_cycles)
{
  return st1wire_platform_get_cycle_count() - start_cycles;
}

/* ---------- Static functions Declarations ---------- */


/**
  * @brief Generates the ST1Wire start-of-frame pulse after detecting an idle bus.
  *
  * @param bus_addr ST1Wire bus address.
  * @return ST1WIRE_OK on success, or ST1WIRE_BUS_ARBITRATION_FAULT if the bus does
  *         not become idle before the timeout.
  */
static st1wire_ReturnCode_t st1wire_send_start(uint8_t bus_addr)
{
  /* Set ST1Wire GPIO in input mode */
  st1wire_platform_io_in(bus_addr);

  /* Wait for the bus to become idle */
  st1wire_platform_start_timeout(ST1WIRE_IDLE);
  while (!st1wire_platform_io_get(bus_addr))
  {
    if (st1wire_platform_is_timeout_exceeded())
    {
      return ST1WIRE_BUS_ARBITRATION_FAULT;
    }
  }

  /* Set ST1Wire GPIO in output mode */
  st1wire_platform_io_out(bus_addr);

  /* Send Start of Frame pulse */
  st1wire_platform_io_clear(bus_addr);
  st1wire_platform_delay(ST1WIRE_START_PULSE);
  st1wire_platform_io_set(bus_addr);

  return ST1WIRE_OK;
}

/**
  * @brief Receives a byte from the ST1Wire bus.
  *
  * @param bus_addr ST1Wire bus address.
  * @param rcv_byte Pointer to store the received byte.
  * @return ST1WIRE_OK on success, or ST1WIRE_BUS_RECEIVE_TIMEOUT if a timeout occurs.
  */
static st1wire_ReturnCode_t st1wire_receive_byte(uint8_t bus_addr, uint8_t *rcv_byte)
{
  uint32_t i = 0, bit_high, bit_low, byteReceived = 0, bit_start;
  uint32_t timeout_cycles;

  /* Perform inter-byte delay */
  st1wire_platform_delay(ST1WIRE_INTER_BYTE_DELAY);

  /* Start critical section */
  ST1WIRE_START_CRITICAL_SECTION
  timeout_cycles = ST1WIRE_RECEIVE_TIMEOUT * st1wire_platform_get_cycles_per_us();

  st1wire_platform_io_out(bus_addr);

  /* Send sync bit('1') */
  st1wire_platform_io_set(bus_addr);
  st1wire_platform_delay(ST1WIRE_LONG_PULSE);
  st1wire_platform_io_clear(bus_addr);
  st1wire_platform_delay(ST1WIRE_LONG_PULSE);
  st1wire_platform_io_set(bus_addr);

  /* Set ST1Wire GPIO in input mode */
  st1wire_platform_io_in(bus_addr);

  /* Handle byte reception */
  for (i = 0; i < 8; i++)
  {
    /* Clear SW counters for bit high and low duration */
    bit_high = 0;
    bit_low = 0;

    /* Measure bit high duration in real time (independent of compiler optimization). */
    bit_start = st1wire_platform_get_cycle_count();
    while (st1wire_platform_io_get(bus_addr)) /* while line value is high*/
    {
      bit_high = st1wire_elapsed_cycles(bit_start);
      if (bit_high >= timeout_cycles)
      {
        ST1WIRE_END_CRITICAL_SECTION
        return ST1WIRE_BUS_RECEIVE_TIMEOUT;
      }
    }
    bit_high = st1wire_elapsed_cycles(bit_start);

    /* Measure bit low duration. */
    bit_start = st1wire_platform_get_cycle_count();
    while (!(st1wire_platform_io_get(bus_addr)))
    {
      bit_low = st1wire_elapsed_cycles(bit_start);
      if (bit_low >= timeout_cycles)
      {
        ST1WIRE_END_CRITICAL_SECTION
        return ST1WIRE_BUS_RECEIVE_TIMEOUT;
      }
    }
    bit_low = st1wire_elapsed_cycles(bit_start);

    /* Store bit value depending on High/low duration */
    if (bit_high > bit_low)
    {
      byteReceived += 1;
    }
    byteReceived <<= 1;
  }
  /* don't do the last shift */
  byteReceived >>= 1;

  /* Acknowledge the byte reception */
  st1wire_platform_io_out(bus_addr);
  st1wire_platform_io_clear(bus_addr);
  st1wire_platform_delay(ST1WIRE_ACK_PULSE);
  st1wire_platform_io_set(bus_addr);

  /* Store the received byte */
  *rcv_byte = (uint8_t)byteReceived;

  /* Exit critical section */
  ST1WIRE_END_CRITICAL_SECTION

  /* Return success */
  return ST1WIRE_OK;
}

/**
  * @brief Sends a byte over the ST1Wire bus.
  *
  * @param bus_addr ST1Wire bus address.
  * @param byte The byte to send.
  * @return ST1WIRE_OK on success, or ST1WIRE_BUS_ACK_ERROR if an ACK error occurs.
  */
static st1wire_ReturnCode_t st1wire_send_byte(uint8_t bus_addr, uint8_t byte)
{
  uint32_t i, timeout_start, timeout_cycles;

  /* Perform inter-byte delay */
  st1wire_platform_delay(ST1WIRE_INTER_BYTE_DELAY);

  /* Enter critical section */
  ST1WIRE_START_CRITICAL_SECTION

  /* Set the STWire GPIO to output mode */
  st1wire_platform_io_out(bus_addr);

  /* Send sync bit('1') */
  st1wire_platform_io_set(bus_addr);
  st1wire_platform_delay(ST1WIRE_SHORT_PULSE);
  st1wire_platform_io_clear(bus_addr);
  st1wire_platform_delay(ST1WIRE_LONG_PULSE);

  /* Send Byte */
  for (i = 0; i < 8; i++)
  {
    /* Mask each bit value*/
    if (byte & (1 << (7 - i)))
    {
      /* Send '1' symbol */
      st1wire_platform_io_set(bus_addr);
      st1wire_platform_delay(ST1WIRE_LONG_PULSE);
      st1wire_platform_io_clear(bus_addr);
      st1wire_platform_delay(ST1WIRE_SHORT_PULSE);
    }
    else
    {
      /* Send '0' symbol */
      st1wire_platform_io_set(bus_addr);
      st1wire_platform_delay(ST1WIRE_SHORT_PULSE);
      st1wire_platform_io_clear(bus_addr);
      st1wire_platform_delay(ST1WIRE_LONG_PULSE);
    }
  }
  /* Release the STWire line */
  st1wire_platform_io_set(bus_addr);

  /* Set the STWire GPIO to input mode to read ACK */
  st1wire_platform_io_in(bus_addr);
  timeout_cycles = ST1WIRE_RECEIVE_TIMEOUT * st1wire_platform_get_cycles_per_us();

  /* Wait for SE ACK (low level on STWire) */
  timeout_start = st1wire_platform_get_cycle_count();
  while (st1wire_platform_io_get(bus_addr))
  {
    if (st1wire_elapsed_cycles(timeout_start) >= timeout_cycles)
    {
      ST1WIRE_END_CRITICAL_SECTION
      return ST1WIRE_BUS_ACK_ERROR;
    }
  }

  /* Wait for SE ACK release */
  timeout_start = st1wire_platform_get_cycle_count();
  while (!(st1wire_platform_io_get(bus_addr)))
  {
    if (st1wire_elapsed_cycles(timeout_start) >= timeout_cycles)
    {
      ST1WIRE_END_CRITICAL_SECTION
      return ST1WIRE_BUS_ACK_ERROR;
    }
  }

  /* exit critical section */
  ST1WIRE_END_CRITICAL_SECTION

  /* Return success */
  return ST1WIRE_OK;
}


/* ---------- Exported functions Declarations ---------- */

st1wire_ReturnCode_t st1wire_SendFrame(uint8_t bus_addr,
                                                uint8_t dev_addr,
                                                uint8_t speed,
                                                uint8_t *frame,
                                                uint16_t frame_length)
{
  st1wire_ReturnCode_t ret;
  uint8_t recv_byte;
  uint16_t i = 0;

  /* Suppress unused parameter warnings */
  (void)dev_addr;
  (void)speed;

  /* Get bus Arbitration and send Start of frame */
  ret = st1wire_send_start(bus_addr);
  if (ret != ST1WIRE_OK)
  {
    return ST1WIRE_BUS_ARBITRATION_FAULT;
  }

  /* Send Frame length MSB*/
  ret = st1wire_send_byte(bus_addr, ((frame_length >> 8) & 0b111));
  if (ret != ST1WIRE_OK)
  {
    return ret;
  }

  /* Send Frame length LSB*/
  ret = st1wire_send_byte(bus_addr, (frame_length & 0xFF));
  if (ret != ST1WIRE_OK)
  {
    return ret;
  }

  /* Send Frame content */
  for (i = 0; i < frame_length; i++)
  {
    ret = st1wire_send_byte(bus_addr, frame[i]);
    if (ret != ST1WIRE_OK)
    {
      return ret;
    }
  }
  /* Get Frame Ack */
  ret = st1wire_receive_byte(bus_addr, &recv_byte);
  if ((ret == ST1WIRE_OK) && (recv_byte != 0x20))
  {
    ret = ST1WIRE_BUS_ACK_ERROR;
  }
  return ret;
}

st1wire_ReturnCode_t st1wire_ReceiveFrame(uint8_t bus_addr,
                                  uint8_t dev_addr,
                                  uint8_t speed,
                                  uint8_t *frame,
                                  uint16_t *pframe_length)
{
  volatile st1wire_ReturnCode_t ret = ST1WIRE_BUS_ACK_ERROR;
  volatile uint16_t i;
  uint8_t rcv_byte;

  /* Suppress unused parameter warnings */
  (void)dev_addr;
  (void)speed;

  /* Get bus Arbitration and send Start of frame */
  ret = st1wire_send_start(bus_addr);
  if (ret != ST1WIRE_OK)
  {
    return ret;
  }

  /* Request Frame reception (frame length MSB = 0x00) */
  ret = st1wire_send_byte(bus_addr, 0x00);
  if (ret != ST1WIRE_OK)
  {
    return ret;
  }

  /* Request Frame reception (frame length LSB = 0x00) */
  ret = st1wire_send_byte(bus_addr, 0x00);
  if (ret != ST1WIRE_OK)
  {
    return ret;
  }

  /* Receive Frame Ack */
  ret = st1wire_receive_byte(bus_addr, &rcv_byte);
  if ((ret != ST1WIRE_OK) || (rcv_byte != 0x20))
  {
    return ST1WIRE_BUS_ACK_ERROR;
  }

  /* Get Frame length  MSB*/
  ret = st1wire_receive_byte(bus_addr, &rcv_byte);
  if (ret != ST1WIRE_OK)
  {
    return ret;
  }
  *pframe_length = rcv_byte << 8;

  /* Get Frame length  LSB*/
  ret = st1wire_receive_byte(bus_addr, &rcv_byte);
  if (ret != ST1WIRE_OK)
  {
    return ret;
  }
  *pframe_length += rcv_byte;

  /* Receive Frame content */
  for (i = 0; i < *pframe_length; i++)
  {
    ret = st1wire_receive_byte(bus_addr, frame + i);
    if (ret != ST1WIRE_OK)
    {
      return ret;
    }
  }

  return ret;
}


void st1wire_wake(uint8_t busID)
{

  /* Execute st1wire wake call-back (platform abstraction)*/
  st1wire_platform_wake(busID);
}

st1wire_ReturnCode_t st1wire_init(uint8_t busID)
{

  /* Suppress unused parameter warnings */
  (void)busID;

  /* Execute st1wire init call-back (platform abstraction)*/
  return st1wire_platform_init();
}

st1wire_ReturnCode_t st1wire_deinit(uint8_t busID)
{
  /* Suppress unused parameter warnings */
  (void)busID;

  /* Execute st1wire init call-back (platform abstraction)*/
  return st1wire_platform_deinit();
}





