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

Every data buffer starts with `DUMMY_WORD_LENGTH` (10) dummy words followed by `BUFFER_HEADER_LENGTH` (12)
header words, all 32-bit. Slot positions are the `BUFFER_HEADER_*_POS` defines in `include/MS_definitions.h`;
the values are written by `setBufferHeader()` in `src/MS_util.c`. miniscope-io strips the preamble, so its
index is the firmware slot minus one.

| slot | name | contents |
|---|---|---|
| 0 | HEADER_LENGTH | `PREAMBLE_WORD` 0x12345678 with `PREAMBLE_ENABLE`, else the header length |
| 1 | MCU_TEMP | MCU die temperature, signed, 0.01 degC (see `MCU_TEMP_ENABLE`) |
| 2 | FRAME_NUM | frame index |
| 3 | BUFFER_COUNT | buffers produced since recording start |
| 4 | FRAME_BUFFER_COUNT | buffer index within the frame |
| 5 | WRITE_BUFFER_COUNT | buffers handed to the transmitter |
| 6 | DROPPED_BUFFER_COUNT | `droppedBufferCount` + `sdoOverrunCount` |
| 7 | TIMESTAMP | ms since recording start |
| 8 | DATA_LENGTH | payload bytes in this buffer |
| 9 | WRITE_TIMESTAMP | ms at SD-card write; TX ring diagnostics on the optical path with `TX_SLIP_TELEMETRY_ENABLE` |
| 10 | BATTERY_VOLTAGE | bits 7:0 battery ADC raw, 8-bit; bits 31:8 header CRC (see `HEADER_CRC_ENABLE`) |
| 11 | WPT_VOLTAGE | bits 7:0 wireless-power input ADC raw; bits 31:24 firmware version record byte (see `VERSION_SIDEBAND_ENABLE`) |

### HEADER_CRC_ENABLE and VERSION_SIDEBAND_ENABLE

No pins. Two fields for the host that use header bits which were unused until now, so older miniscope-io
versions keep decoding the stream (they only need to mask slot 10 to its low 8 bits for the battery value).
Rig test data (flashed firmware binary and mio stream CSV): `test_data/`.

**Header CRC** (`HEADER_CRC_ENABLE`): `setBufferHeader()` fills all 12 slots, then computes CRC-32 (IEEE,
identical to Python's `zlib.crc32`) over the 12 header words as little-endian bytes with slot 10 bits 31:8
zero, and stores the low 24 bits in slot 10 bits 31:8. The host recomputes it and drops a buffer whose CRC
does not match, so a single flipped bit in `frame_num` or `buffer_count` can no longer mis-sequence a frame.
Nibble-table implementation in `src/MS_header_info.c`, about 10 us per buffer. Only valid on the optical path;
the SD path rewrites header slots after `setBufferHeader()`.

```python
words[10] &= 0xFF                      # the CRC field counts as zero
crc_ok = (zlib.crc32(struct.pack("<12I", *words)) & 0xFFFFFF) == received_slot10 >> 8
```

**Firmware version record** (`VERSION_SIDEBAND_ENABLE`): the device streams from power-on and the host attaches
at any time, so instead of a one-off announcement a 32-byte record repeats forever, one byte per buffer in
slot 11 bits 31:24, byte index = `bufferCount % 32`. Any 32 consecutive buffers (4 frames, 0.2 s) give the
whole record. Built once in `startRecording()` by `buildVersionRecord()`; version numbers live in
`include/MS_version.h`, the git hash in `MS_version_git.h` written by `script/MS_prebuild.ps1` (0 = unknown,
e.g. a build that skipped the pre-build step). Purpose: let the host check that firmware, mio version and
FPGA bitfile are a known-good combination before data is lost.

| byte | content |
|---|---|
| 0 | magic 0xA5 |
| 1 | record format, 1 |
| 2, 3, 4 | firmware version major, minor, patch |
| 5..8 | git short hash, little endian, 0 = unknown |
| 9 | header layout version (2 = MCU temp in slot 1, TX telemetry in slot 9) |
| 10 | flags: bit 0 RTC timestamps, 1 black reference line, 2 TX telemetry, 3 sensor status, 4 MCU temp, 6:5 `BLACKCAL_MODE`, 7 header CRC |
| 11 | `DEVICE_ID` |
| 12, 13 | image width, little endian |
| 14, 15 | image height, little endian |
| 16, 17 | black reference pixels per frame, little endian |
| 18 | frame rate |
| 19 | `NUM_BUFFERS` |
| 20 | `BUFFER_BLOCK_LENGTH` |
| 21 | git tree dirty at build time |
| 22..30 | reserved, 0 |
| 31 | checksum: two's complement of the sum of bytes 0..30, so all 32 bytes sum to 0 mod 256 |

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

## Documentation

API documentation is generated with Doxygen and committed under `html/`; open `html/index.html` in a browser. To regenerate, run `doxygen Doxyfile` in the repository root.

## License

This project is licensed under the GNU Affero General Public License v3.0; see [LICENSE](LICENSE). The modified ASF driver files in `ASF_custom/` are derived from the Atmel Software Framework and retain their original Microchip/Atmel license headers.
