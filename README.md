# STSE host platform – NUCLEO-L452RE

Platform drivers to run [STSELib](https://github.com/STMicroelectronics/STSELib) on a NUCLEO-L452RE board, with I2C and ST1Wire support.

The main difference from the other host platforms is the ST1Wire driver. It uses a timer and DMA to generate and sample the bus instead of bit-banging with interrupts disabled.

## Content

| Folder | Content |
| --- | --- |
| `Core/` | CMSIS, startup files (GCC/IAR), linker scripts, `SystemInit` (64 MHz, peripheral clocks, USART2 and I2C1 pins) |
| `Drivers/` | Bare-metal drivers: `crc16`, `delay_ms`, `delay_us`, `i2c`, `rng`, `st1wire`, `uart` |
| `STSELib/` | STSELib platform abstraction (`stse_platform_*.c`) connecting the library to the drivers |

## Board resources

| Function | Resource |
| --- | --- |
| ST1Wire line | PA9 (open-drain, no internal pull-up, so an external pull-up is required) |
| ST1Wire engine | TIM1 CH1/CH2, DMA1 channels 2 and 3 |
| I2C | I2C1, PB8 (SCL) / PB9 (SDA) |
| SE power control | PB0, PC0, PC1 (low = powered) |
| µs/ms delays and timeouts | TIM6 (shared by `delay_us` and `delay_ms`) |
| Console | USART2, PA2/PA3 (ST-LINK virtual COM port) |

The ST1Wire driver reserves TIM1 and DMA1 channels 2/3, so the application must not use them.

## ST1Wire driver

Only the 3C mode is supported.

### Why timer + DMA

The reference driver bit-bangs every bit and measures device pulses by polling, so it disables interrupts (and suspends the scheduler under FreeRTOS) for each byte. Any interrupt that fires during a byte would corrupt the timing.

Here, each byte is turned into a list of edge times and TIM1 drives the line from that list:

- **TIM1 CH2** (PA9, AF1) is in output compare toggle mode. **DMA1 channel 3** loads the next toggle time into `CCR2` after each match.
- **TIM1 CH1** captures both edges of TI2, which is the same pin, and **DMA1 channel 2** stores the timestamps. This captures the edges driven by the device as well as the host's own.
- TIM1 runs at 1 MHz, so every time value is in microseconds.

The CPU only waits for the DMA to finish, with interrupts left enabled. Interrupt latency only delays when the result is read and does not affect the waveform. Once the transfer is done, the captured timestamps are used to:

- **Transmit**: check that the device acknowledged the byte (2 extra edges after the last bit).
- **Receive**: decode each bit by comparing its high and low durations.

The start condition and the wake-up pulse are long and not timing critical, so they are still driven with the pin as a GPIO.

### Byte format (3C)

```
 ----+      +-----------+   +---+          +- ... -+   +----
     |      |           |   |   |          |       |   |
     +------+           +---+   +----------+       +---+
       sync     bit = 1         bit = 0              ack
```

| Parameter | Value |
| --- | --- |
| Short pulse | 7 µs |
| Long pulse | 13 µs |
| Bit `1` / `0` | high long + low short / high short + low long |
| Sync | low for a long pulse |
| Host ack (receive) | 2 µs low, 1 µs after the 8th bit |
| Start condition | 72 µs low |
| Bus idle before start | 100 µs high |
| Inter-byte delay | 10 µs |
| Byte timeout | 2 ms |
| Wake-up | 1 ms low, then 8 ms wait |

### Frame format

```
Host → device:  START [ADDR] LEN_MSB LEN_LSB DATA...         ← STATUS (0x20)
Device → host:  START [ADDR] 0x00 0x00  ← STATUS (0x20) LEN_MSB LEN_LSB DATA...
```

`ADDR` is only sent when the device address is not 0. Reading a response is done by sending an empty frame.

### Structure

```
STSELib/stse_platform_st1wire.c   STSELib glue, frame buffering
        │
Drivers/st1wire/st1wire.c         frame layer: header, payload, status
        │
Drivers/st1wire/st1wire_phy.c     3C timings, byte encoding/decoding, start, wake-up
        │
Drivers/st1wire/st1wire_platform.c  PA9, TIM1, DMA1, µs time base (STM32L452 specific)
```

Porting to another pin or MCU only requires `st1wire_platform.c`: it must play a list of toggle times on the line and timestamp every edge.

### Limitations

- 3C only, single bus (the STSELib `busID` and `speed` arguments are ignored).
- The CPU still busy-waits during a byte (around 200 µs), but interrupts stay enabled.
- When receiving, the host ack is placed at the nominal end of the byte instead of being synchronised on the device edges.

## Code style

Formatting is enforced with [pre-commit](https://pre-commit.com/) (clang-format, 4-space indent):

```
pip install pre-commit
pre-commit install
pre-commit run --all-files
```
