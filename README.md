minimal64
---------
An emulator written in plain C capable of running Commodore 64 programs which do not use the Kernal, BASIC or Character ROMs.
It has its own 252 byte custom Kernal and does not have a BASIC or Character ROM included.

The intention of minimal64 is to run new software, not the back catalog of C64 software.

If a PRG/CRT runs on minimal64, it should run on real C64 hardware or C64 Emulators.

Datasette, Disk Drives, analog color encoding, CRT effects, the user port and parallel port are not emulated.

The Expansion/Cartridge port only supports a limited selection of cartridge formats (see below).

No platform specific libraries used, should compile with most c compilers.

Aims
----
Small: compiles to 90KB of JavaScript (36KB when gzipped)

Cycle Accurate

Well documented: each section in the source has a notes folder - still in progress..

Custom Kernal
-------------
minimal64 has its own 252 byte kernal located at memory location $e000. The entry point is $e000 and the cold reset vector at $fffc is set to this address.

The aim of the kernal is just to allow PRGs or CRTs to be run.

If no cartridge is loaded, the kernal will setup up some things such as the stack pointer, the pal/ntsc flag, and an interrupt to update the jiffy clock. It will then loop on a black screen waiting for a prg.

If a cartridge is loaded, the kernal will disable interrupts, setup the stack pointer and jump to the vector stored at $8000

see src/kernal/kernal.asm for details

BASIC ROM and Character ROM are also banked in, but have no data in them (all zeros).

Obviously if a program is written which relies on this custom Kernal, it will not run on real hardware.
It's better to just bank out the ROMs as soon as possible.

User Supplied Kernal/BASIC/Character ROMS
-----------------------------------------
There are functions to load in user supplied Kernal/BASIC/Character ROMS

Formats Supported
-----------------
PRG - if using the minimal64 kernal, minimal64 will look in the PRG for the BASIC SYS token and an address to jump to
    - if a different kernal has been loaded in, minimal64 will attempt to put 'RUN:' and the RETURN character into 
      the keyboard buffer after injecting the prg
CRT - formats supported: Standard 8KB/16KB Cartridges, Ocean Type 1, C64GS, Magic Desk

Color Palettes
--------------
minimal64 does not emulate PAL/NTSC color encoding/CRT effects.
Instead it uses RGB values for each of the 16 colors of the c64.
By default, it uses the Colodore Palatte for PAL and VICE's Pepto NTSC palette for NTSC.
There are functions to set the RGB values for each of the 16 colors.

API
---
For an API, see example/m64-wrappers.js

Example
-------
see example/index.html

A simple web page with file controls to allow loading of PRG and CRT files.

Credits
-------
Author nopsta 2022

Mostly based on the Java c64 emulator within JSIDPlay2, Sidplay 2, VICE, reSID-fp and http://www.zimmers.net/anonftp/pub/cbm/documents/
