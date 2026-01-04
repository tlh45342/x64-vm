NASM ?= nasm
NASMFLAGS ?= -f bin
EMU ?= x64-vm.exe

ASM := two_ints.asm
BIN := two_ints.bin

LOAD_ADDR ?= 0x0000
CS ?= 0x0000
IP ?= 0x1000
MAX_STEPS ?= 256

EMU_ARGS := --bin $(BIN) --load-addr $(LOAD_ADDR) --cs $(CS) --ip $(IP) --max-steps $(MAX_STEPS)

all: $(BIN)

$(BIN): $(ASM)
	$(NASM) $(NASMFLAGS) $< -o $@

test: $(BIN)
	$(EMU) $(EMU_ARGS)

clean:
	-del /q $(BIN) 2>nul || exit 0

.PHONY: all test clean