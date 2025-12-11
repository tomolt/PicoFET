# PicoFET

Cheap MSP430 programmers for hobbyist or classroom use can be hard to come by.
This project is built upon unmodified Raspberry Pi Pico (or compatible) MCUs, which are not only very affordable, but are also available at many electronics retailers.

## Features

- JTAG pin interface: TDI, TDO, TCK, TMS, TST (plus RST pin)
- Accessing register contents, RAM
- Reading, erasing, writing the flash memory
- Single-stepping the processor

**Notably absent:**
- Support for non-RP2XYZ boards (But codebase is mostly independent of HW details)
- Blowing the JTAG security fuse (Neither hobbyist-friendly nor doable with on-board voltage supply)
- Spy-bi-Wire
- Breakpoints

## Installing

### Using picotool

First, download, compile, and install [picotool](https://github.com/raspberrypi/picotool).

Hold the BOOTSEL button on your Pico while you connect its (micro) USB port to your computer.

From within your build folder, execute the following commands:

```sh
picotool load PicoFET.uf2
picotool verify PicoFET.uf2
picotool reboot
```

## Building from Source

## Contributing

## Credits

PicoFET was written by Thomas Oltmann.

It incorporates parts of Daniel Beer's mspdebug.

The software JTAG implementation was written by Peter Bägel.
