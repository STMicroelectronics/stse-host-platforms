/**
 ******************************************************************************
 * \file    st1wire_phy.c
 * \brief   ST1Wire physical layer (3C)
 ******************************************************************************
 *
 * A byte is a sync pulse followed by 8 bits, MSB first. Each bit is a high
 * phase then a low phase: long/short for a 1, short/long for a 0.
 * The receiver of the byte acknowledges with a short low pulse.
 *
 *  ----+      +-----------+   +---+          +- ... -+   +----
 *      |      |           |   |   |          |       |   |
 *      +------+           +---+   +----------+       +---+
 *        sync     bit = 1         bit = 0              ack
 *
 * Bytes are not bit-banged: the expected waveform is described as a list of
 * toggle times and the platform timer plays it while timestamping every edge.
 */

#include "st1wire_phy.h"
#include "st1wire_platform.h"

/* Timings in microseconds */
#define ST1WIRE_SHORT_PULSE 7U
#define ST1WIRE_LONG_PULSE 13U
#define ST1WIRE_BIT_PERIOD (ST1WIRE_SHORT_PULSE + ST1WIRE_LONG_PULSE)
#define ST1WIRE_ACK_PULSE 2U
#define ST1WIRE_START_PULSE 72U
#define ST1WIRE_IDLE 100U
#define ST1WIRE_INTER_BYTE_DELAY 10U
#define ST1WIRE_BYTE_TIMEOUT 2000U
#define ST1WIRE_WAKE_PULSE 1000U
#define ST1WIRE_WAKE_DELAY 8000U

/* Edges of a byte: 2 for the sync pulse, 2 per bit */
#define ST1WIRE_BYTE_EDGES 18U
#define ST1WIRE_ACK_EDGES 2U

/* When receiving, the host drives the sync pulse and, once the device has
 * sent its 8 bits, the ack pulse */
#define ST1WIRE_RX_DATA_END (2U * ST1WIRE_LONG_PULSE + 8U * ST1WIRE_BIT_PERIOD)
#define ST1WIRE_RX_ACK_START (ST1WIRE_RX_DATA_END + 1U)
#define ST1WIRE_RX_ACK_END (ST1WIRE_RX_ACK_START + ST1WIRE_ACK_PULSE)

static uint16_t rx_toggles[] = {
    ST1WIRE_LONG_PULSE,
    2U * ST1WIRE_LONG_PULSE,
    ST1WIRE_RX_ACK_START,
    ST1WIRE_RX_ACK_END,
};

static uint16_t tx_toggles[ST1WIRE_BYTE_EDGES];
static volatile uint16_t edges[ST1WIRE_BYTE_EDGES + ST1WIRE_ACK_EDGES];

/* Returns once the line has stayed high for ST1WIRE_IDLE */
static void wait_bus_idle(void) {
    for (;;) {
        st1wire_platform_timeout_start(ST1WIRE_IDLE);
        while (st1wire_platform_io_get() != 0U) {
            if (st1wire_platform_timeout_expired() != 0U) {
                return;
            }
        }
    }
}

static st1wire_ReturnCode_t wait_edges(void) {
    st1wire_platform_timeout_start(ST1WIRE_BYTE_TIMEOUT);
    while (st1wire_platform_edges_remaining() != 0U) {
        if (st1wire_platform_timeout_expired() != 0U) {
            return ST1WIRE_BUS_RECEIVE_TIMEOUT;
        }
    }
    return ST1WIRE_OK;
}

static st1wire_ReturnCode_t wait_waveform_time(uint16_t time) {
    st1wire_platform_timeout_start(ST1WIRE_BYTE_TIMEOUT);
    while (st1wire_platform_waveform_time() <= time) {
        if (st1wire_platform_timeout_expired() != 0U) {
            return ST1WIRE_BUS_RECEIVE_TIMEOUT;
        }
    }
    return ST1WIRE_OK;
}

void st1wire_phy_init(void) {
    st1wire_platform_init();
}

void st1wire_phy_deinit(void) {
    st1wire_platform_deinit();
}

st1wire_ReturnCode_t st1wire_phy_send_start(void) {
    st1wire_platform_io_in();
    wait_bus_idle();

    if (st1wire_platform_io_get() == 0U) {
        return ST1WIRE_BUS_ARBITRATION_FAULT;
    }

    st1wire_platform_io_out();
    st1wire_platform_io_clear();
    st1wire_platform_delay_us(ST1WIRE_START_PULSE);
    st1wire_platform_io_set();

    return ST1WIRE_OK;
}

st1wire_ReturnCode_t st1wire_phy_send_byte(uint8_t byte) {
    st1wire_ReturnCode_t ret;
    uint16_t time = 0U;
    uint16_t high;
    uint16_t low;
    uint8_t n = 0U;
    uint8_t mask;

    time += ST1WIRE_SHORT_PULSE;
    tx_toggles[n++] = time;
    time += ST1WIRE_LONG_PULSE;
    tx_toggles[n++] = time;

    for (mask = 0x80U; mask != 0U; mask >>= 1U) {
        if ((byte & mask) != 0U) {
            high = ST1WIRE_LONG_PULSE;
            low = ST1WIRE_SHORT_PULSE;
        } else {
            high = ST1WIRE_SHORT_PULSE;
            low = ST1WIRE_LONG_PULSE;
        }
        time += high;
        tx_toggles[n++] = time;
        time += low;
        tx_toggles[n++] = time;
    }

    /* Also capture the device ack, which follows the last bit */
    st1wire_platform_waveform_start(tx_toggles, ST1WIRE_BYTE_EDGES,
                                    edges, ST1WIRE_BYTE_EDGES + ST1WIRE_ACK_EDGES);
    ret = wait_edges();
    st1wire_platform_waveform_stop();

    if ((ret != ST1WIRE_OK) ||
        (edges[ST1WIRE_BYTE_EDGES + 1U] <= edges[ST1WIRE_BYTE_EDGES])) {
        return ST1WIRE_BUS_ACK_ERROR;
    }

    return ST1WIRE_OK;
}

st1wire_ReturnCode_t st1wire_phy_receive_byte(uint8_t *byte) {
    st1wire_ReturnCode_t ret;
    uint16_t rising;
    uint16_t falling;
    uint16_t next_rising;
    uint8_t value = 0U;
    uint8_t i;

    st1wire_platform_waveform_start(rx_toggles, sizeof(rx_toggles) / sizeof(rx_toggles[0]),
                                    edges, ST1WIRE_BYTE_EDGES);
    ret = wait_edges();
    if (ret == ST1WIRE_OK) {
        ret = wait_waveform_time(ST1WIRE_RX_ACK_END);
    }
    st1wire_platform_waveform_stop();

    if (ret != ST1WIRE_OK) {
        return ret;
    }

    /* edges[0..1] are the sync pulse, then a falling/rising pair per bit */
    rising = edges[1];
    for (i = 0U; i < 8U; i++) {
        falling = edges[2U + 2U * i];
        next_rising = edges[3U + 2U * i];

        value <<= 1U;
        if ((uint16_t)(falling - rising) > (uint16_t)(next_rising - falling)) {
            value |= 1U;
        }
        rising = next_rising;
    }

    *byte = value;

    return ST1WIRE_OK;
}

void st1wire_phy_inter_byte_delay(void) {
    st1wire_platform_delay_us(ST1WIRE_INTER_BYTE_DELAY);
}

void st1wire_phy_wake(void) {
    st1wire_platform_io_out();
    st1wire_platform_io_clear();
    st1wire_platform_delay_us(ST1WIRE_WAKE_PULSE);
    st1wire_platform_io_set();
    st1wire_platform_delay_us(ST1WIRE_WAKE_DELAY);
}
