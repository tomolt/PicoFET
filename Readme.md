# PicoFET

_PicoFET_ is an open-source firmware for the Raspberry Pi Pico and compatible
boards that turns them into debuggers & programmers for TI MSP430 family MCUs.
No additional hardware components are required (other than some wires).

Through this project, I attempt to provide a cheap, accessible, and easily sourced
means of tinkering with MSP430 ICs to hobbyists and classrooms.

This project does not replace all of the functionality that commercial solutions offer,
nor does it provide any guarantees or certifications.

## Features

- JTAG pin interface: TDI, TDO, TCK, TMS, TST (plus RST pin)
- Accessing register contents, RAM
- Reading, erasing, writing the flash memory
- Single-stepping the processor

**Notably absent:**
- Support for non-RP2XYZ boards (But codebase is mostly independent of HW details)
- Blowing the JTAG security fuse (Neither hobbyist-friendly nor doable with on-board voltage supply)
- Spy-bi-Wire (Pull Requests welcome)
- Breakpoints (Pull Requests welcome)
- Not tested against MSP430Fxxx or MSP430FRxxx series (Feedback welcome)

## Installing

First, download, compile, and install [picotool](https://github.com/raspberrypi/picotool).

Hold the BOOTSEL button on your Pico while you connect its (micro) USB port to your computer.

From within your build folder, execute the following commands:

```sh
picotool load PicoFET.uf2
picotool verify PicoFET.uf2
picotool reboot
```

## Building from Source

First, install a C++ Compiler and CMake, and download [pico-sdk](https://github.com/raspberrypi/pico-sdk).
Execute `git submodule update --init` inside the pico-sdk directory.

Inside the PicoFET directory, do:
- In a shell, type `export PICO_SDK_PATH=` followed by the file path to your pico-sdk directory.
- Create a new directory, e.g. `build`.
- Inside that new directory, call CMake. You will have to specify your processor and board here.
  E.g.
  ```sh
  cmake .. -DCMAKE_BUILD_TYPE=Release -DPICO_PLATFORM=rp2350 -DPICO_BOARD=pico2_w
  ```
  or
  ```sh
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DPICO_PLATFORM=rp2040 -DPICO_BOARD=pico
  ```
  Consult the pico-sdk and CMake documentation for more information.
- Run
  ```sh
  make
  picotool uf2 convert PicoFET.elf PicoFET.uf2
  ```
- You can now flash the `PicoFET.uf2` image onto your board.

## Connecting Pins

The following tables list the default pinout depending on the type of board.
Board pin numbers are numbered in the same order as as is customary for DIP chips.

If you wish to use a type of board that is not listed here, or you need a different
pin assingment, you will have to either edit `src/pinout.h`, or pass your pin assignments
to CMake as such:
```sh
cmake -DPIN_TCK=8 -DPIN_TMS=9 {etc. for all non-power pins; your other cmake options follow here}
```
In either case, you will have to build PicoFET from source.

### Pi Pico / Pico W / Pico 2 / Pico 2 W

| Board Pin No. | GPIO Pin No. | JTAG Pin Function |
| ------------: | -----------: | :---------------- |
|            11 |            8 | TCK               |
|            12 |            9 | TMS               |
|            13 |            - | VSS (GND)         |
|            14 |           10 | TST               |
|            15 |           11 | ~RST              |
|            16 |           12 | TDO               |
|            17 |           13 | TDI (TCLK)        |
|            36 |            - | VCC               |

### Seeed XIAO RP2040 / RP2350

| Board Pin No. | GPIO Pin No. | JTAG Pin Function |
| ------------: | -----------: | :---------------- |
|             2 |           27 | TCK               |
|             3 |           28 | TMS               |
|             4 |            5 | TDO               |
|             5 |            6 | TDI (TCLK)        |
|            10 |            4 | ~RST              |
|            11 |            3 | TST               |
|            12 |            - | VCC               |
|            13 |            - | VSS (GND)         |

## Credits

PicoFET was written by Thomas Oltmann.

It incorporates parts of Daniel Beer's mspdebug.

The software JTAG implementation was written by Peter Bägel.
