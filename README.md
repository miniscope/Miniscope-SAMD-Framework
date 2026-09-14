## README
See documentation by cloning this repository and locally opening [html/index.html] with browser.
- To do: host this somewhere after making this public.

### Tips
- Use this as a Git submodule.
    - If you're not building a Atmel project inside a git repository, you can do a normal clone too.
- Only edit the files in Miniscope-SAMD-Framework
    - To modify ASF drivers, add modified driver files to ASF_custom and add file path to ```MS_prebuild.ps1``` and ```MS_pre_reconfig.ps1``` as ```$cfilepatharray``` and ```$headerpatharray```.
    - Don't directly modify ATMEL START generated driver files.
- Follow git flow (main, dev, feature)
    - You need to make a branch **inside** the nested git submodule.

### Repository structure
- src: custom functions
- include: header files for files in src
- ASF_custom: customized ASF drivers
- script: scripts for taking care of conflicts with Atmel START

### Buffer header
Every data buffer starts with `DUMMY_WORD_LENGTH` (10) dummy words followed by
`BUFFER_HEADER_LENGTH` (12) header words, all 32-bit. Slot positions are the
`BUFFER_HEADER_*_POS` defines in `MS_definitions.h`; the values are written by
`setBufferHeader()` in `MS_util.c`.

| slot | name | contents |
|---|---|---|
| 0 | HEADER_LENGTH | `PREAMBLE_WORD` 0x12345678 with `PREAMBLE_ENABLE`, else the header length |
| 1 | LINKED_LIST | `bufferCount % NUM_BUFFERS` |
| 2 | FRAME_NUM | frame index |
| 3 | BUFFER_COUNT | buffers produced since recording start |
| 4 | FRAME_BUFFER_COUNT | buffer index within the frame |
| 5 | WRITE_BUFFER_COUNT | buffers handed to the transmitter |
| 6 | DROPPED_BUFFER_COUNT | buffers skipped; on the optical path, buffers lost when the camera laps the transmitter |
| 7 | TIMESTAMP | ms since recording start |
| 8 | DATA_LENGTH | payload bytes in this buffer |
| 9 | WRITE_TIMESTAMP | ms at SD-card write; unused on the optical path, see below |
| 10 | BATTERY_VOLTAGE | battery ADC raw, 8-bit |
| 11 | WPT_VOLTAGE | wireless-power input ADC raw, 8-bit |

Slot order and size are unchanged, so miniscope-io parses these buffers unmodified (its
index is the firmware slot minus one, since the preamble is stripped).

`TX_SLIP_TELEMETRY_ENABLE` in `MS_definitions.h` repurposes slot 9 for TX ring
diagnostics, packed as `(maxBacklog << 24) | (resyncCount << 16) | (phaseErr << 8) |
phaseRef`. The drift `phaseErr - phaseRef` must stay near 0; at `NUM_BUFFERS` slots the
transmitter reads the buffer the camera is overwriting. Comment the flag out for
production recordings.

### How to configure the git submodule
1. Set up an Atmel START project
    - Make sure to follow the Peripheral requirements stated below.
2. Go to the directory including main.c using git bash (or equivalent shell) 
3. Execute the following command
```bash
git submodule add https://github.com/Aharoni-Lab/Miniscope-SAMD-Framework ./MS_module
```
Might need to do the following here. (not sure if this is safe though)
```bash
git config --global protocol.file.allow always
```
4. In Solution Explorer (Microchip Studio), click "Show All Files".
5. Right click on ```MS_module``` and select "Include in Project"
6. Add following to the include path (Project -> Properties -> Toolchain -> ARM/GNU C Compiler -> Directories) (configuration: All configurations)
```
../MS_module/include
```
7. Add pre-build eventBuild events (configuration: All configurations)
```ps
powershell.exe -ExecutionPolicy Bypass -NoProfile -NonInteractive -File "..\MS_module\script\MS_prebuild.ps1"
```

### Conditional compile flag
- All mode/peripheral enable should be defined in MS_definitions.h
    - Firmware mode: end by _MODE or _TESTMODE
    - Peripheral mode  : end by _ENABLE or _DISABLE
- Conditional compile should be flagged by _ENABLE or _DISABLE

#### Mode flag
- Select/define one of the modes using #define
- End mode with _MODE or _TESTMODE
```c
// ------ FIRMWARE MODE ------------------------
#define WLMS_MODE
//#define V4WF_MODE
//#define DMA_TO_SPI_TESTMODE
//#define DMA_TO_SPI_METRO_TESTMODE
```

#### Peripheral enable flag
- Select/define the enabled peripherals using #ifdef or #if
- End with _ENABLE or _DISABLE
```c
// ------ PERIPHERAL ENABLE ------------------------
#ifdef WLMS_MODE
#define PYTHON480_ENABLE
#define DMA_TO_SPI_ENABLE
#define EXLED_PWM_ENABLE
#define BATTERY_ENABLE
#define WPT_ADC_ENABLE
#define EWL_ENABLE
#define PUSH_BUT_ENABLE
#define STATUS_LED_ENABLE
#define 

#endif
```

#### Conditional compile
Define peripheral functions within a peripheral enable flag (avoid using mode flags)

```c
#ifdef PYTHON480_ENABLE
// Enable the 3.3V regulator
gpio_set_pin_level(EN_3V3, true);
I2C_BB_init();
#endif
```

```c
#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM7_ENABLE)
hri_sercomspi_set_CTRLC_ICSPACE_bf(SERCOM7, SPI_ICSPACE_MS);
hri_sercomspi_write_BAUD_reg(SERCOM7, SPI_BAUD_MS);
spi_m_sync_enable(&
);
#endif
   ```

#### SERCOM for DMA
- Define SERCOM setting in ATMEL START
- Only part that should be manually changed is the DMA's DSTADDR.reg in MS_dma.c. This should be defined using conditional compile.
```c
#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM0_ENABLE)
TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM0->SPI.DATA.reg;
#endif
```

### Peripheral requirements (Atmel START config)
#### PYTHON480_ENABLE
Drivers
```
TIMER_0
CAMERA_0
EXTERNAL_IRQ_0
```

Pins
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

#### BATTERY_ENABLE
```
BATT_VOLT
```

#### WPT_ENABLE
```
WPT_VOLT
```

#### EWL_ENABLE
```
I2C_BB_SCL
I2C_BB_SDA
```

#### IR_TRIGGER_ENABLE
```
IR_RX
```

#### IR_UART_ENABLE
```
IR_RX
```

#### EXLED_PWM_ENABLE
```
LED_PWM
ENT_LED
```

### Open questions / to do
- Write everything for minimum prototype
- Ask someone to add module
    - I think Marcel is interesting in adding the LUTmodule
- More safe/efficient coding rule
- Branch protection
- Code organization
    - Probably the functions should be organized by peripherals? or Projects?
    - Might need to narrow down namespaces
- Good way to add error handling (if we need it)
- Namespace is probably too wide than it should be
    - Almost all global now
    - Might be ok for this scale project
- If conditional compile should be defined in main.c or in each function
