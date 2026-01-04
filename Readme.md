# x64-vm

A tiny, growing x86 (real-mode) emulator used to bootstrap “PC-style” bring-up: executing raw `.bin` test programs, basic BIOS-ish behavior, and eventually booting from a floppy image.

This repo is intentionally incremental: we add just enough instruction + platform support to keep moving forward (tests first, then boot path).

---

## Status

### Working today
- Loads and executes 16-bit `.bin` programs at a chosen `CS:IP`.
- Test harness layout similar to `libvm` (smoke + core instruction-set tests).
- Basic UART-style output via **COM1** (`OUT DX, AL` to port `0x3F8`).
- Interrupt basics: IVT, `INT`, `IRET` (good for BIOS-like interrupt services later).
- “Boot fallback”: if no `--bin` is given, the emulator will try to boot from a file named **`floopy.img`** (reads sector 0 into `0x7C00` and starts at `0000:7C00`).

### In progress
- Booting a real FreeDOS 1.44MB image: the emulator is already executing the boot sector and relocating (you’ll see `CS` become `1FE0` during the far jump). We’re progressively adding the required opcodes and addressing modes as they appear.

### Next major milestone
- **INT 13h disk services** (starting with AH=02h CHS reads) backed by `floopy.img`, so the boot sector can load `KERNEL.SYS` and continue toward `COMMAND.COM`.

---

## Repo Layout

```
x64-vm/
  Makefile
  src/
    main.c
    x64_cpu.c
    x64_cpu.h
  rom/
    shim_bios.asm        # early experiments / placeholders
  tests/
    00-smoke/
      mov_add.asm
      Makefile
    01-core-iset/
      001_mov_ax/
      002_mov_all_regs/
      ...
      012_two_ints/
      ...
Build
Prerequisites
A C compiler (GCC recommended)

make

nasm

On Windows, a MinGW-w64/WinLibs style toolchain works well (as long as gcc, make, and nasm are on PATH).

Build the emulator
From the repo root:

bat
Copy code
cd O:\x64-vm
make
This produces:

text
Copy code
bin\x64-vm.exe
Install (Windows)
This project is installed to the same prefix family as libvm.

Default prefix:

PREFIX := C:/Program Files/libvm

Install:

bat
Copy code
make install
After install, you should be able to run:

bat
Copy code
x64-vm
Tip: if you’re debugging and keep seeing “old behavior,” run where x64-vm to confirm which binary is being executed (PATH vs local bin\).

Running
Run a raw .bin test
Typical test execution:

bat
Copy code
x64-vm --bin mov_ax_imm.bin --load-addr 0x1000 --cs 0x0000 --ip 0x1000 --max-steps 32
Common output format:

text
Copy code
HALT=1 ERR=0 AX=beef BX=0000 CX=0000 DX=0000 CS:IP=0000:1004
Meaning:

HALT=1: CPU reached HLT

ERR=1: an unhandled opcode or fatal execution error occurred

Registers shown are 16-bit (real-mode focus right now)

CS:IP is the final location

Boot from floopy.img (default boot path)
If you run with no arguments:

bat
Copy code
x64-vm
The emulator will attempt:

open floopy.img in the current directory

read LBA 0 (512 bytes) → load to physical 0x7C00

set CS:IP = 0000:7C00

execute

This is the easiest way to iterate on “boot sector bring-up.”

Tests
Each test directory is self-contained and supports:

bat
Copy code
make
make test
Example:

bat
Copy code
cd tests\01-core-iset\001_mov_ax
make test
Test philosophy
Tests are tiny .asm programs assembled to flat binaries (-f bin) and executed by the emulator.

We add an instruction when a test needs it (or when boot code hits it).

Keep tests named and staged so progress is obvious (00-smoke, 01-core-iset, etc.).

Implemented Instruction Coverage (high-level)
This will evolve quickly, but currently includes (at least):

MOV r16, imm16

ADD basics used by early tests

JMP rel8

CALL rel16, RET

PUSH/POP r16

CMP subset (0x83 /7 ib reg-direct) + JZ/JNZ subset (as added)

INT / IRET + IVT basics

OUT DX, AL for COM1-style output

Boot-path support opcodes as encountered (CLI/STI, CLD/STD, NOP, REP MOVSW, JMP FAR, etc.)

Early 16-bit ModRM / addressing support as required by boot sector progression

If you hit:

text
Copy code
x86_step: unhandled opcode 0x??
…that opcode becomes the next “work item,” and we’ll usually add a focused test or confirm it appears in boot code.

Roadmap
Near-term
Stabilize 16-bit ModRM memory addressing (BP+disp, BX+SI, etc.)

Add INT 13h (disk) backed by floopy.img

Add minimal “BIOS-ish” interrupt services needed by DOS bootstrapping

Add CLI/REPL commands (planned):

version

regs

load <bin>

do <script>

tracing toggles

Longer-term
A minimal “shim BIOS” ROM and reset vector behavior

CMOS/RTC modeling (eventually)

More complete instruction coverage (still guided by tests + boot needs)

Devices: timer, keyboard, basic text mode video (later)

Notes / Conventions
For now, we call the floppy image floopy.img (intentionally spelled that way). We can generalize later (e.g., --fd0 / --disk).

This project is intentionally pragmatic: correctness is improved iteratively as tests and boot code require.

License
TBD / choose a license for the repo (MIT/BSD-2/Apache-2 are common choices).

pgsql
Copy code

If you want, I can also generate a matching `tests/README.md` that documents the test numbering scheme and a template `Makefile` for new instruction tests.
::contentReference[oaicite:0]{index=0}
```



THIS IS SO VERY VERY FLIMSY AND BEGINING - PLEASE EXCUSE.
THIS HAS VERY SHARP EDGES AND IS VERY MUCH A WORK IN PROGRESS.
THIS IS A GIANT SHIM OF SHIMS AND IS MESSY WORK IN PROGRESS.