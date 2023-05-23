# Coding rules
The goal is to:
- Run all ATSAMD51-based devices with the same code using conditional compile.
- Allow automatic configuration updates using Atmel START (no more manual driver file update selection and fixing).

## General rules
- Use this as a Git submodule. Don't directly build the c project in the repository.
- Never directly modify ATMEL START generated driver files. Driver modifications should be defined in "MS_driver_update.c".
- Follow git flow [to do: add link]

## How to configure the Miniscope submodule
1. Set up an Atmel START project
    - Make sure to follow the Peripheral requirements stated below.
2. Go to the directory including main.c using git bash (or equivalent shell) 
3. Execute the following command
```
git submodule add https://[github-username]@github.com/Aharoni-Lab/Miniscope_SAMD_modules
```
Might need to do the following here. (not sure if safe)
```
git config --global protocol.file.allow always
```
4. 

## 


## Peripheral requirements (Atmel START config)
- PYTHON480_ENABLE
    - I2C_BB_SCL
    - I2C_BB_SDA
- PYTHON480_ENABLE
    - I2C_BB_SCL
    - I2C_BB_SDA
- BATTERY_ADC_ENABLE
    - BATT_VOLT
- EXLED_PWM_ENABLE
    - LED_PWM
- DMA_TO_SPI_ENABLE
    - 

## Global variables
- Define in MS_util.c
- Declare in MS_definitions.h

## Functions
- All application specific functions should be declared in MS_definitions.h
File locations
- MS_util.c: for utility functions
- MS_cb.c: callback functions
- MS_dma.c: project specific DMA functions
- MS_drive_update.c: for adding drivers (currently empty)
- MS_record.c: for recording fuction
- dma_util (.c and .h): driver functions added in v4WF project

## Conditional compiling
- All mode/peripheral enable should be defined in MS_definitions.h
    - Firmware mode: end by _MODE or _TESTMODE
    - Peripheral mode  : end by _ENABLE or _DISABLE
- Conditional compile should be flagged by _ENABLE or _DISABLE

## SERCOM for DMA
- Define SERCOM setting in ATMEL START
- Only part that should be manually changed is the DMA's DSTADDR.reg in MS_dma.c. This should be defined using conditional compile.
```
#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM0_ENABLE)
TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM0->SPI.DATA.reg;
#endif
```

## Declarations
- Application specific function declerations in MS_definitions.h

## Open questions
- Namespace
- If conditional compile should be defined in main.c or not