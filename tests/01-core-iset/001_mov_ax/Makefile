NASM ?= nasm
NASMFLAGS ?= -f bin

EMU ?= x64-vm.exe

ASM := mov_ax_imm.asm
BIN := mov_ax_imm.bin

all: $(BIN)

$(BIN): $(ASM)
	$(NASM) $(NASMFLAGS) $< -o $@

test: $(BIN)
	python run_tests.py

clean:
	-del /q $(BIN) 2>nul || exit 0

.PHONY: all test clean