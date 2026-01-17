# x64-vm top-level Makefile (Windows-friendly)

CC = gcc

# Include paths:
#   -Isrc         lets you include like: "cpu/cpu.h", "vm/vm.h", "cli/repl.h"
#   -Isrc/include lets you include like: "version.h"
CFLAGS ?= -Wall -Wextra -O2 -std=c11 -Isrc -Isrc/include

NASM ?= nasm
NASMFLAGS ?= -f bin

# Install prefix (override: make PREFIX="C:/Program Files/libvm" install)
PREFIX ?= C:/Program Files/libvm

# GNU coreutils shim (matches libvm style). Override if needed.
GNU_BIN ?= C:/Program Files/GNU/bin
MKDIR_P ?= "$(GNU_BIN)/mkdir.exe" -p
RM_RF   ?= "$(GNU_BIN)/rm.exe" -rf
CP      ?= "$(GNU_BIN)/cp.exe" -f

BIN_DIR   := bin
BUILD_DIR := build

X64VM := $(BIN_DIR)/x64-vm.exe

# --- sources ---------------------------------------------------------------

CLI_SRCS  := $(wildcard src/cli/*.c)
CPU_SRCS  := $(wildcard src/cpu/*.c)
VM_SRCS   := $(wildcard src/vm/*.c)
UTIL_SRCS := $(wildcard src/util/*.c)

# If you keep attic/scratch files around, exclude them here.
CPU_SRCS := $(filter-out src/cpu/bloat.c src/cpu/old.c,$(CPU_SRCS))

SRCS := src/main.c $(CLI_SRCS) $(VM_SRCS) $(UTIL_SRCS) $(CPU_SRCS)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_BIN := tests/00-smoke/mov_add.bin

all: $(X64VM)

# --- directories -----------------------------------------------------------

$(BIN_DIR):
	@$(MKDIR_P) "$@"

# --- compile/link ----------------------------------------------------------

# Single object rule: always mkdir the output directory.
$(BUILD_DIR)/%.o: %.c
	@$(MKDIR_P) "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

$(X64VM): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# --- tests -----------------------------------------------------------------

tests/00-smoke/%.bin: tests/00-smoke/%.asm
	$(NASM) $(NASMFLAGS) $< -o $@

test: all
	$(X64VM) --bin $(TEST_BIN) --load-addr 0x1000 --cs 0x0000 --ip 0x1000 --max-steps 32

# --- install ---------------------------------------------------------------

install: $(X64VM)
	@$(MKDIR_P) "$(PREFIX)/bin"
	@$(CP) "$(X64VM)" "$(PREFIX)/bin/"

clean:
	@$(RM_RF) $(BUILD_DIR) $(BIN_DIR)

.PHONY: all test install clean