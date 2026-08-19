# -------- Toolchain --------
CC      := arm-none-eabi-gcc
GDB     := arm-none-eabi-gdb
READELF := arm-none-eabi-readelf
NM      := arm-none-eabi-nm
OBJDUMP := arm-none-eabi-objdump
QEMU    := qemu-system-arm
RENODE  := renode

# -------- Target --------
CPU   := cortex-m3
BOARD := mps2-an385

# Board to build/debug under Renode -- independent of BOARD above, which
# is QEMU-specific (`make qemu` only supports mps2-an385; there's no
# QEMU machine model for stm32f103 wired up here). stm32f103 is now the
# main target board; override with RENODE_BOARD=mps2-an385 to build for
# the older QEMU-compatible target instead.
RENODE_BOARD ?= stm32f103

# -------- Paths --------
BUILD_DIR := build
ELF       := $(BUILD_DIR)/rtos.elf
APP_SRC   ?= app/app_main.c

ifeq ($(RENODE_BOARD),stm32f103)
LD_SCRIPT            := linker/stm32f103.ld
BSP_SRC               := bsp/stm32f103/systick.c
RENODE_RUN_SCRIPT     := scripts/renode/stm32f103_run.resc
RENODE_DEBUG_SCRIPT   := scripts/renode/stm32f103_debug.resc
else
LD_SCRIPT            := linker/mps2-an385.ld
BSP_SRC               := bsp/qemu_mps2/systick.c
RENODE_RUN_SCRIPT     := scripts/renode/run.resc
RENODE_DEBUG_SCRIPT   := scripts/renode/debug.resc
endif

# -------- Flags --------
CFLAGS  := -mcpu=$(CPU) -mthumb -O0 -g -ffreestanding -nostdlib -I.
LDFLAGS := -T $(LD_SCRIPT)

# -------- Sources (edit to match your tree) --------
SRCS := \
	arch/arm_cm/startup.c \
	arch/arm_cm/port.c \
	kernel/main.c \
	$(APP_SRC) \
	benchmark/bench_clock.c \
	kernel/tick.c \
	kernel/task.c \
	kernel/scheduler.c \
	kernel/memory.c \
	kernel/mutex.c \
	kernel/semaphore.c \
	kernel/mempool.c \
	kernel/queue.c \
	kernel/lfqueue.c \
	kernel/kalloc.c \
	$(BSP_SRC)

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: build check-board bench-sched qemu qemu-bench-sched renode renode-debug renode-run debug sections segments symbols disasm clean

# ---------- Build ----------
# build/rtos.elf's path doesn't vary by RENODE_BOARD or APP_SRC, but its
# actual link inputs (LD_SCRIPT, BSP_SRC, APP_SRC) do -- make can't detect
# a Makefile *variable* changing between invocations via mtimes alone, so
# switching either without this would silently relink against stale
# objects from whichever combination was last built. This tracks the
# last-built (board, app) pair and wipes the build dir if it changed.
BOARD_MARKER := $(BUILD_DIR)/.board_marker
BUILD_ID := $(RENODE_BOARD)|$(APP_SRC)

check-board:
	mkdir -p $(BUILD_DIR)
	if [ -f $(BOARD_MARKER) ] && [ "$$(cat $(BOARD_MARKER))" != "$(BUILD_ID)" ]; then \
		echo "Board/app changed ($$(cat $(BOARD_MARKER)) -> $(BUILD_ID)) -- cleaning stale build"; \
		rm -rf $(BUILD_DIR); \
		mkdir -p $(BUILD_DIR); \
	fi
	echo "$(BUILD_ID)" > $(BOARD_MARKER)

build: check-board $(ELF)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJS) $(LD_SCRIPT)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

# ---------- Run / Debug ----------
qemu: build
	$(QEMU) -M $(BOARD) -cpu $(CPU) -nographic -kernel $(ELF) -S -s

# NOTE: QEMU's mps2-an385 model does not implement the DWT unit, so
# DWT_CYCCNT (used by bench/bench_clock.c) always reads back 0 under qemu.
# Use `make renode` for cycle-count-based benchmarking.
renode: build
	$(RENODE) --disable-gui $(RENODE_RUN_SCRIPT)

renode-debug: build
	$(RENODE) --disable-gui $(RENODE_DEBUG_SCRIPT)

# GDB port must match `machine StartGdbServer` in scripts/renode/debug.resc.
RENODE_GDB_PORT := 3333
RENODE_LOG      := $(BUILD_DIR)/renode.log

# Starts Renode in the background (log at $(RENODE_LOG)), waits for its GDB
# server to come up, attaches gdb, then kills Renode when gdb exits.
renode-run: build
	if lsof -nP -iTCP:$(RENODE_GDB_PORT) -sTCP:LISTEN >/dev/null 2>&1; then \
		echo "Port $(RENODE_GDB_PORT) is already in use -- a Renode session from" >&2; \
		echo "a previous run is likely still alive (its cleanup only fires on a" >&2; \
		echo "clean gdb exit, not if you Ctrl-C the whole 'make renode-run')." >&2; \
		echo "Find it:  lsof -nP -iTCP:$(RENODE_GDB_PORT)" >&2; \
		echo "Kill it:  pkill -f Renode.dll" >&2; \
		exit 1; \
	fi; \
	$(RENODE) --disable-gui $(RENODE_DEBUG_SCRIPT) > $(RENODE_LOG) 2>&1 & \
	RENODE_PID=$$!; \
	trap "kill $$RENODE_PID 2>/dev/null" EXIT INT TERM; \
	until lsof -nP -iTCP:$(RENODE_GDB_PORT) -sTCP:LISTEN >/dev/null 2>&1; do \
		if ! kill -0 $$RENODE_PID 2>/dev/null; then \
			echo "Renode exited early; see $(RENODE_LOG)" >&2; \
			exit 1; \
		fi; \
		sleep 0.2; \
	done; \
	$(GDB) $(ELF) -ex "target remote 127.0.0.1:$(RENODE_GDB_PORT)"; \
	kill $$RENODE_PID 2>/dev/null

debug: build
	$(GDB) $(ELF)

# ---------- Inspection ----------
sections: build
	$(READELF) -S $(ELF)

segments: build
	$(READELF) -l $(ELF)

symbols: build
	$(NM) $(ELF)

disasm: build
	$(OBJDUMP) -d $(ELF)

# ---------- Cleanup ----------
clean:
	rm -rf $(BUILD_DIR)
