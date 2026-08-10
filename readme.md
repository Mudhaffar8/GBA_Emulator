# Game Boy Advance Emulator
This is a WIP Game Boy Advance Emulator.

# Showcase
![Hello World](./showcase/hello_demo.png) ![Mode 3](./showcase/mode3_demo2.png) ![Mode 4 Keypad](./showcase/keypad_demo2.png) ![Mode 4 Keypad](./showcase/bigmap_demo.png)

## Current Features
- A fully-working ARM7TDMI cpu interpreter, passing the [FUZZARM](https://github.com/DenSinH/FuzzARM) rom test and [SingleStepTest's ARM7TDMI Test Suite<sup>1</sup>](https://github.com/SingleStepTests/ARM7TDMI). 
- A decoupled ARM7TDMI disassembler.
- SDL3 for window and input handling.
- Basic support for Bit Map Modes 3 and 4.

<i>
    1. With the exception of the result of the carry flag for any MULT Instruction. 
    The result of the carry flag is UNPREDICTABLE (per the ARM7TDMI documentation and GBATEK) 
    and no exisitng GBA game (to my knowledge) relies on it, so this emulator either sets it to a garbage value
    or leaves it be.
</i>

## Planned Updates
- Continue working on and completing the PPU:
    - Adding Modes 0, 1, and 2
    - Adding the sprite layer and all additional sprite attributes 
    - Adding post-processing effects (i.e. alpha blending, mosaic)
    - Fixing Mode 5
- An ImGui-based debugger that allows you to view:
    - CPU GP registers
    - Disassembler showing the program trace
    - Current mode and CPSR
    - Interrupt I/O registers (IE, IF, IME)
    - EWRAM/IWRAM hex dump
- Implement Timers
- Implement DMA
- Make/CMake Compilation


## Note
- This emulator requires a BIOS to run. I mostly used [Nebuleon's Open Source BIOS replacement](https://github.com/Nebuleon/ReGBA/tree/master) for my emulator so I'd recommend using that one.

This project is in active development, so stay tuned for updates :)
