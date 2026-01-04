# x64-vm top-level Makefile (Windows-friendly)

CC = gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c11 -Isrc

NASM ?= nasm
NASMFLAGS ?= -f bin

# Install prefix (can override: make PREFIX="C:/Program Files/libvm" install)
PREFIX ?= C:/Program Files/libvm

# GNU coreutils shim (matches libvm style). Override if needed.
GNU_BIN ?= C:/Program Files/GNU/bin
MKDIR_P ?= "$(GNU_BIN)/mkdir.exe" -p
RM_RF   ?= "$(GNU_BIN)/rm.exe" -rf
CP      ?= "$(GNU_BIN)/cp.exe" -f

BIN_DIR   := bin
BUILD_DIR := build

X64VM := $(BIN_DIR)/x64-vm.exe

CPU_SRCS := $(wildcard src/cpu/*.c)
SRCS     := src/main.c src/repl.c src/vm.c $(CPU_SRCS)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_BIN := tests/00-smoke/mov_add.bin

all: $(X64VM) $(TEST_BIN)

# --- directories ---
$(BIN_DIR):
	@$(MKDIR_P) $@

$(BUILD_DIR):
	@$(MKDIR_P) $@

$(BUILD_DIR)/src:
	@$(MKDIR_P) $@

# --- compile/link ---
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR) $(BUILD_DIR)/src
	$(CC) $(CFLAGS) -c $< -o $@

$(X64VM): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# --- tests ---
tests/00-smoke/%.bin: tests/00-smoke/%.asm
	$(NASM) $(NASMFLAGS) $< -o $@

test: all
	$(X64VM) --bin $(TEST_BIN) --load-addr 0x1000 --cs 0x0000 --ip 0x1000 --max-steps 32

# --- install ---
install: $(X64VM)
	@$(MKDIR_P) "$(PREFIX)/bin"
	@$(CP) "$(X64VM)" "$(PREFIX)/bin/"

clean:
	@$(RM_RF) $(BUILD_DIR) $(BIN_DIR) tests/00-smoke/*.bin

.PHONY: all test install clean
