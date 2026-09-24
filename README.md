# Miniscope-SAMD-Framework

Firmware framework for wireless miniature microscopes (miniscopes) based on Microchip SAM D51 microcontrollers. The framework reads out a CMOS image sensor (ON Semiconductor PYTHON 480 or AMS NanEye) through the parallel capture controller (PCC), buffers image data in MCU RAM, and streams it via DMA to a serial data link (SPI/USART) or an SD card. It also controls miniscope peripherals such as the electrowetting lens (EWL), excitation LED, battery and wireless-power voltage monitoring, an IR receiver for remote control, and status LEDs.

The framework is designed to be used as a git submodule inside a Microchip Studio (Atmel START) project. Pre-build scripts link the framework sources into the generated project so that Atmel START files are never edited directly.

## Repository structure

- `src/`: application sources
- `include/`: headers, including the build configuration (`MS_config.h`)
- `ASF_custom/`: customized Atmel Software Framework (ASF) drivers, stored with `.csrc`/`.hsrc` extensions and installed by the pre-build script
- `script/`: PowerShell scripts that install/uninstall the framework sources into the parent Atmel START project
- `atstart/`: Atmel START configuration archives (`.atzip`) for supported hardware
- `main.csrc`: application entry point, installed as the parent project's `main.c`
- `Doxyfile`, `html/`: Doxygen documentation; open `html/index.html` in a browser after cloning

## Requirements

- Microchip Studio with an Atmel START project targeting a SAM D51 device
- PowerShell (invoked by the pre-build event)

## Setup

1. Create an Atmel START project configured with the drivers and pins listed under [Peripheral requirements](#peripheral-requirements-atmel-start-config). The archives in `atstart/` can be used as a starting point.
2. From the directory containing the project's `main.c`, add this repository as a submodule:
   ```bash
   git submodule add https://github.com/Aharoni-Lab/Miniscope-SAMD-Framework ./MS_module
   ```
3. In the Microchip Studio Solution Explorer, click "Show All Files", right-click `MS_module`, and select "Include in Project".
4. Add the following to the include paths (Project -> Properties -> Toolchain -> ARM/GNU C Compiler -> Directories, configuration: All configurations):
   ```
   ../MS_module/include
   ```
5. Add the following pre-build event (configuration: All configurations):
   ```
   powershell.exe -ExecutionPolicy Bypass -NoProfile -NonInteractive -File "..\MS_module\script\MS_prebuild.ps1"
   ```

The pre-build script replaces the generated `main.c` and selected ASF drivers with the versions in this repository (`main.csrc`, `ASF_custom/`) using hard links. Before re-running Atmel START code generation, run `script/MS_pre_reconfig.ps1` to restore the original files.

Do not modify Atmel START generated driver files directly. To customize an ASF driver, add the modified file to `ASF_custom/` (with a `.csrc`/`.hsrc` extension) and register its path in `$cfilepatharray`/`$headerpatharray` in `MS_prebuild.ps1` and `MS_pre_reconfig.ps1`.

## Configuration

All build configuration is done with compile-time flags in `include/MS_config.h`.

### Mode flags

Select exactly one hardware mode (`*_MODE` or `*_TESTMODE`), which determines the data path and the set of peripherals compiled in:

```c
// ------ HARDWARE MODE ------------------------
//#define V4WF_MODE
#define WLMS_SPI_MODE
//#define BERT_MODE
//#define GS_MODE
//#define WLMS_USART_MODE
//#define WLMS_SD_MODE
//#define DMA_TO_SPI_TESTMODE
//#define DMA_TO_SPI_METRO_TESTMODE
```

For example, `WLMS_SPI_MODE` streams PYTHON 480 image data over SPI, `WLMS_SD_MODE` records to an SD card, and `BERT_MODE` streams a PRBS test pattern for bit-error-rate testing of the data link.

### Peripheral flags

Each mode defines the peripherals to enable (`*_ENABLE` / `*_DISABLE`) together with mode-specific parameters (buffer sizes, frame rate, sensor ROI, device ID, etc.):

```c
#ifdef WLMS_SPI_MODE
#define PYTHON480_ENABLE
#define EWL_ENABLE
#define DMA_TO_SPI_ENABLE
#define EXLED_PWM_ENABLE
#define BATTERY_ENABLE
// ...
#endif
```

### Conditional compilation

Peripheral code should be guarded by the peripheral's `_ENABLE` flag (not by mode flags):

```c
#ifdef PYTHON480_ENABLE
gpio_set_pin_level(EN_3V3, true); // Enable the 3.3V regulator
I2C_BB_init();
#endif
```

### SERCOM for data output

The SERCOM used for serial data output is configured in Atmel START. On the firmware side, select the corresponding `SPI_SERCOMx_ENABLE` or `USART_SERCOMx_ENABLE` flag in the mode definition; the DMA destination register is resolved from this flag (see `src/MS_global_variable.c`).

### Buffer header

Every data buffer starts with `DUMMY_WORD_LENGTH` (10) dummy words followed by
`BUFFER_HEADER_LENGTH` (12) header words, all 32-bit. Slot positions are the
`BUFFER_HEADER_*_POS` defines in `include/MS_definitions.h`; the values are written by
`setBufferHeader()` in `src/MS_util.c`. miniscope-io strips the preamble, so its index is the
firmware slot minus one.

| slot | name | contents |
|---|---|---|
| 0 | HEADER_LENGTH | `PREAMBLE_WORD` 0x12345678 with `PREAMBLE_ENABLE`, else the header length |
| 1 | MCU_TEMP | MCU die temperature, signed, 0.01 degC (see `MCU_TEMP_ENABLE`) |
| 2 | FRAME_NUM | frame index |
| 3 | BUFFER_COUNT | buffers produced since recording start |
| 4 | FRAME_BUFFER_COUNT | buffer index within the frame |
| 5 | WRITE_BUFFER_COUNT | buffers handed to the transmitter |
| 6 | DROPPED_BUFFER_COUNT | `droppedBufferCount` + `sdoOverrunCount` (see below) |
| 7 | TIMESTAMP | ms since recording start |
| 8 | DATA_LENGTH | payload bytes in this buffer |
| 9 | WRITE_TIMESTAMP | ms at SD-card write; 0 on the optical path unless `TX_SLIP_TELEMETRY_ENABLE` |
| 10 | BATTERY_VOLTAGE | battery ADC raw, 8-bit |
| 11 | WPT_VOLTAGE | bits 7:0 wireless-power input ADC raw; bits 31:8 sensor status (see `SENSOR_STATUS_ENABLE`) |

### Dropped buffers on the optical path

`droppedBufferCount` only moves in `sdmmc_dma_transfer_control()`, and both its call site and
its timer registration sit inside `#if 0`, so on the optical link it is always 0. What that path
can actually lose is a buffer the camera overwrites before the transmitter has sent it:
`countTxOverrun()` (called for every filled buffer, in `pcc_dma_cb()` and at frame end in
`frameValid_cb()`) keeps one `sdoUnsentMask` bit per ring slot, set on fill and cleared when the
transmitter starts that slot, and counts one `sdoOverrunCount` whenever a slot is refilled with its
bit still set. Verified exact on hardware with a forced TX stall (140 counted vs 140 buffers missing
on the host). Header slot 6 carries the
sum of the two, so the field means "buffers lost" on either path and miniscope-io needs no
change. Should stay 0; a non-zero value means the transmitter fell more than `NUM_BUFFERS`
behind the camera and image data was lost.

### TX_SLIP_TELEMETRY_ENABLE

Defined per mode in `include/MS_config.h`. Puts optical TX ring diagnostics in header slot 9,
one byte each, MSB first: `maxBacklog | skippedResume | phaseErr | slipCount`.

- `phaseErr`: hardware TX ring slot minus `writeBufferCount`, mod `NUM_BUFFERS`. Must stay 0.
- `slipCount`: times `phaseErr` changed.
- `skippedResume`: resumes refused because the previous TX block was still in flight.
- `maxBacklog`: deepest transmit backlog seen, in buffers.

## Peripheral requirements (Atmel START config)

### PYTHON480_ENABLE

Drivers:

```
TIMER_0
CAMERA_0
EXTERNAL_IRQ_0
```

Pins:

```
SPI_BB_SCK
SPI_BB_MOSI
SPI_BB_MISO
SPI_BB_NSS
PCC_D0,...,PCC_D7
PCC_CLK
PCC_HV
PCC_FV
FrameValid
EN_3V3
RESET_CMOS
MONITOR0
GCLK1_OUT
```

### BATTERY_ENABLE

```
BATT_VOLT
```

### WPT_ADC_ENABLE

```
WPT_VOLT
```

### EWL_ENABLE

```
I2C_BB_SCL
I2C_BB_SDA
```

### IR_TRIGGER_ENABLE / IR_UART_ENABLE

```
IR_RX
```

### EXLED_PWM_ENABLE

```
LED_PWM
ENT_LED
```

### MCU_TEMP_ENABLE

No pins. Requires `BATTERY_ENABLE` (ADC_0 on ADC0): the SAM D51 temperature sensor (SUPC PTAT/CTAT) is only reachable through ADC0. `readMCUTemperature()` reconfigures ADC0 for the measurement and restores the battery-ADC settings afterwards. The result is written into `BUFFER_HEADER_MCU_TEMP_POS` in the buffer header (signed int32, 0.01 degC, `MCU_TEMP_INVALID` if unavailable), replacing the old DMA linked-list position field, which was always equal to `buffer count % NUM_BUFFERS` and unused on the host side.

The temperature is only sampled every `MCU_TEMP_READ_PERIOD_TICKS` battery-check ticks (default 4, i.e. every 2 s) because the read blocks the timer ISR for ~0.4 ms; battery and WPT monitoring stay at 500 ms.

### SENSOR_STATUS_ENABLE

No pins (uses the sensor's bit-banged SPI). Makes the PYTHON480 black-level calibration visible, since
wireless-power disturbance on the black lines is the suspect for frame-wide "whitening" offsets. The
computed black offset itself is not readable, so header slot 11 bits 31:8 carry what is:

| slot 11 bits | contents |
|---|---|
| 7:0 | `wptVolt` (unchanged) |
| 15:8 | sensor die temperature, reg 97 raw (~0.75 degC/LSB, uncalibrated), refreshed every `SENSOR_TEMP_READ_PERIOD_FRAMES` (40) frames |
| 17:16 | `blackcal_error` per data channel, reg 136: not enough black samples, calibration invalid |
| 19:18 | `BLACKCAL_MODE` in effect: 0 auto, 1 freeze, 2 manual |
| 22 | regs 96/128/129 read back different from what was written |
| 23 | valid: 1 once `applyBlackCalMode()` ran; 0 in older firmware |

`readSensorStatus()` runs in `frameValid_cb()` at end of frame, after the PCC is re-armed, with a faster
bit-bang read (`SENSOR_SPI_HALF_PERIOD_US` = 2, ~120 us per register; the stock `spi_BB_Read()` takes
~0.6 ms). The status of frame N therefore appears in the headers of frame N+1. `applyBlackCalMode()`
runs at recording start, writes reg 129 for the chosen mode and verifies the configuration.

`BLACKCAL_MODE` in `MS_config.h` selects the A/B test: `AUTO` (0x8001, default), `FREEZE` (0x83FF, meant to
hold the current factors; not documented in the PYTHON family datasheet, verify on hardware) or `MANUAL`
(auto calibration off, fixed `BLACKCAL_MANUAL_OFFSET`, documented behaviour). Expect a few transient frames
after the reg 129 write in FREEZE/MANUAL.

### BLACKREF_LINE_ENABLE

Puts the sensor's actual per-frame black level into the data stream. The PYTHON480 sends one
reference line (reg 207 = 1) after its black lines, filled with the black average that the calibration
block computed for each of the two kernel columns (reg 129[14] `ref_mode`), and reg 130[3] passes it to
the PCC. Before this flag the init already generated 20 reference lines, gated from `line_valid`.

The line is full sensor width (404 kernels = 808 pixels) and comes first in every frame. To keep a
frame inside the same 8 buffers, the image shrinks by one ROI y unit to 200 x 196:
196 * 200 + 808 = 40008 of 8 * 5032 = 40256 pixels. Only sized for `PYTHON480_200PX_NOSUBSAMPLE`.
miniscope-io needs the matching config (`black_ref_px: 808`, `frame_height: 196`), which strips the
line and logs the two channel averages per frame.

### Open issues (feature-blacklevel-header)

Hardware-validated so far (2026-09-22, dev board, battery, 15 min `test_long`): every frame 40,008 px in
8 buffers, frame period unchanged, black reference steady at ~24 counts, unaffected by light, and
tracking gain steps (2x: ~21, 3.5x: ~15).

- [ ] **PYTHON480 temperature readout needs the same validation as `MCU_TEMP_ENABLE` got.** Long run next
  to the MCU temperature, check every update lands on the 40-frame grid, and a raw-to-degC calibration
  (one point at a cold start is the simplest). The PYTHON480 datasheet lists reg 96/97 only as registers:
  the ~0.75 degC/count scale and "96[0] = enable" come from the PYTHON 1300 datasheet (on the 480, 96[0] is
  "reserved"). Test whether the readout changes with 96 = 0, then fix the comments here and in the code.
- [ ] **Move the reg 136 / reg 97 reads out of `frameValid_cb()`** to the 2 s tick in `checkBattVoltage_cb`
  (next to the MCU temperature). The reference line already carries the black level at no MCU cost;
  `blackcal_error` was 0 in all 146,776 headers of `test_long`, so a per-frame read is not needed.
  Scope-time the read (GPIO toggle) either way; the ~120 us is calculated, not measured.
- [ ] **Pin down what the reference-line value means in absolute terms.** It drops as gain rises, so it is
  not simply raw dark level x gain. A short `BLACKCAL_MODE_MANUAL` run with a few known offsets relates
  it to the applied correction.
- [ ] **Frame period:** this branch runs 49.48 ms (20.2 fps), the same as the temperature branch; one earlier
  capture (unknown build) ran 48.67 ms. Find which firmware that was and whether the period changed
  on the way.
- [ ] miniscope-io: commit the reference-line decoder (`black_ref_px`, period-4 kernel-column split, 200x196
  reshape) and the `wireless-200px-blackref` config; currently uncommitted in the mio working tree.

## Documentation

API documentation is generated with Doxygen and committed under `html/`; open `html/index.html` in a browser. To regenerate, run `doxygen Doxyfile` in the repository root.

## License

This project is licensed under the GNU Affero General Public License v3.0; see [LICENSE](LICENSE). The modified ASF driver files in `ASF_custom/` are derived from the Atmel Software Framework and retain their original Microchip/Atmel license headers.
