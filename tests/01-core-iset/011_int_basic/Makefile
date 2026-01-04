NASM ?= nasm
NASMFLAGS ?= -f bin
EMU ?= x64-vm.exe

ASM := int_basic.asm
BIN := int_basic.bin

# Load the whole image at physical 0 so the IVT at 0x0000 works.
LOAD_ADDR ?= 0x0000
CS ?= 0x0000
IP ?= 0x1000
MAX_STEPS ?= 128

EMU_ARGS := --bin $(BIN) --load-addr $(LOAD_ADDR) --cs $(CS) --ip $(IP) --max-steps $(MAX_STEPS)

all: $(BIN)

$(BIN): $(ASM)
	$(NASM) $(NASMFLAGS) $< -o $@

test: $(BIN)
	$(EMU) $(EMU_ARGS)

clean:
	-del /q $(BIN) 2>nul || exit 0

.PHONY: all test clean