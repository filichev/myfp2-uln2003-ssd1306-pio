# myFP2 ULN2003 (PlatformIO, SSD1306 OLED)

This is a PlatformIO-based Arduino Nano (ATmega328P, new bootloader) project for the myFocuserPro2 (release 338) ULN2003 build, configured for a 0.96" 128x64 SSD1306 I2C OLED using the SSD1306Ascii library.

Русский: Это проект на PlatformIO для Arduino Nano (ATmega328P, новый загрузчик), myFocuserPro2 (релиз 338) с драйвером ULN2003 и OLED 0.96" 128x64 SSD1306 (I2C) через библиотеку SSD1306Ascii.

## Features
- Board: ULN2003_M_MT_F_BT (BT.BUZZ.LEDs.LCD.OLED.HPSW.PB.TMP.SPWR)
- Display: OLED 128x64 (SSD1306), via SSD1306Ascii
- Push buttons enabled with long-press speed boost
- Temperature probe enabled (DS18B20)
- PlatformIO workflow (build, upload), VS Code friendly

## Project layout
- `src/` — Firmware sources (config.h, display.h, serialcomms.h, myFP2_ULN2003_338.ino, etc.)
- `lib/mySSD1306Compat/` — Lightweight compatibility headers to map expected mySSD1306 includes/APIs to SSD1306Ascii
- `platformio.ini` — PlatformIO environment (`nanoatmega328new`), dependencies, and upload port

## Build and Upload

Requirements:
- VS Code + PlatformIO extension
- Arduino Nano (ATmega328P, new bootloader)
- USB port (adjust COM port if needed)

Build:
- PlatformIO: click “Build” (or run the PlatformIO build task)

Upload:
- Ensure the correct port is set in `platformio.ini` (default: `upload_port = COM8`)
- Click “Upload” in PlatformIO (or run the upload task)

## Configuration notes
- `src/config.h`
  - `#define DRVBRD  ULN2003_M_MT_F_BT`
  - `#define DISPLAYTYPE DISPLAY_OLED12864`
  - `#define USE_SSD1306 1`
  - `#define PUSHBUTTONS 1` and `#define TEMPERATURE_PROBE 1` are enabled
  - Long-press speed boost is enabled; multiplier can be tuned via `PB_SPEED_BOOST_MULTIPLIER` (default here is 200)

- `src/display.h`
  - Uses SSD1306Ascii (Wire) and adapts “Display_*” behaviors to SSD1306Ascii equivalents (invert/contrast/clear)

- `platformio.ini`
  - Env: `nanoatmega328new`
  - Dependencies: Wire, TimerOne, OneWire, Bounce2, SSD1306Ascii
  - Ignores broken global `NewLiquidCrystal`
  - Adds include path for `lib/mySSD1306Compat/src`

## CI (GitHub Actions)
A simple CI workflow is included to build the firmware with PlatformIO on push/PR.

## Upstream reference
This project aligns to myFocuserPro2 release 338 (Oct-2024). Original project and boards documentation by Robert Brown.

## Troubleshooting
- If build fails due to missing libraries, ensure PlatformIO downloads all dependencies (see `lib_deps` in `platformio.ini`).
- If OLED remains blank, check I2C wiring and address (default `0x3C`).
- If upload fails, close any open serial monitor and confirm the COM port.
