# -------- Toolchain --------
CC      := arm-none-eabi-gcc
GDB     := arm-none-eabi-gdb
READELF := arm-none-eabi-readelf
NM      := arm-none-eabi-nm
OBJDUMP := arm-none-eabi-objdump
RENODE  := renode

# -------- Target --------
CPU   := cortex-m3

# Board to build/debug under Renode. stm32f103 is the main target board;
# override with RENODE_BOARD=stm32f103c8t6 for the "Blue Pill" (64K flash /
# 20K RAM) variant.
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
else ifeq ($(RENODE_BOARD),stm32f103c8t6)
LD_SCRIPT            := linker/stm32f103c8t6.ld
BSP_SRC               := bsp/stm32f103/systick.c
RENODE_RUN_SCRIPT     := scripts/renode/stm32f103c8t6_run.resc
RENODE_DEBUG_SCRIPT   := scripts/renode/stm32f103c8t6_debug.resc
else
$(error Unknown RENODE_BOARD '$(RENODE_BOARD)' -- expected stm32f103 or stm32f103c8t6)
endif

# -------- Real hardware clock bring-up (opt-in) --------
# HW=0 (default): boots on the reset-default 8MHz HSI, never touches RCC.
# Required for Renode -- its platform files (scripts/renode/*.repl) don't
# model RCC, so an HSERDY/PLLRDY poll there spins forever. Renode's own
# SysTick/DWT frequency is fixed at 25MHz regardless of real RCC state
# (see scripts/renode/*.repl), which is what OS_CPU_HZ tracks below.
#
# HW=1: compiles in bsp/stm32f103/clock.c and runs real HSE+PLL bring-up
# to 72MHz before the scheduler starts ticking. Only meaningful when
# flashing real silicon -- e.g. `make build RENODE_BOARD=stm32f103c8t6 HW=1`.
HW ?= 0
ifeq ($(HW),1)
BSP_SRC += bsp/stm32f103/clock.c
CFLAGS  += -DOS_HW_CLOCK_INIT -DOS_CPU_HZ=72000000u
else
CFLAGS  += -DOS_CPU_HZ=25000000u
endif

# -------- Flags --------
CFLAGS  += -mcpu=$(CPU) -mthumb -O0 -g -ffreestanding -nostdlib -I.
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
	kernel/rtt.c \
	$(BSP_SRC)

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: build check-board bench-sched renode renode-debug debug sections segments symbols disasm clean

# ---------- Build ----------
# build/rtos.elf's path doesn't vary by RENODE_BOARD, APP_SRC, or HW, but
# its actual link inputs (LD_SCRIPT, BSP_SRC, APP_SRC, CFLAGS) do -- make
# can't detect a Makefile *variable* changing between invocations via
# mtimes alone, so switching any of them without this would silently
# relink against stale objects from whichever combination was last built.
# This tracks the last-built (board, app, hw) triple and wipes the build
# dir if it changed.
BOARD_MARKER := $(BUILD_DIR)/.board_marker
BUILD_ID := $(RENODE_BOARD)|$(APP_SRC)|$(HW)

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
renode: build
	$(RENODE) --disable-gui $(RENODE_RUN_SCRIPT)

# GDB port must match `machine StartGdbServer` in $(RENODE_DEBUG_SCRIPT).
RENODE_GDB_PORT := 3333
RENODE_LOG      := $(BUILD_DIR)/renode.log

# Starts Renode in the background (log at $(RENODE_LOG)), waits for its GDB
# server to come up, attaches gdb, then kills Renode when gdb exits. If a
# Renode session from a previous run is still holding the GDB port (its
# cleanup only fires on a clean gdb exit, not on Ctrl-C), kill it first.
#
# Deliberately does NOT depend on `build` -- it debugs whatever ELF was
# last built, instead of silently rebuilding (and possibly wiping/relinking
# against a different RENODE_BOARD/APP_SRC than you meant to debug, see
# check-board) every time you attach. Run `make build` yourself first.
renode-debug:
	if [ ! -f $(ELF) ]; then \
		echo "$(ELF) not found -- run 'make build' first" >&2; \
		exit 1; \
	fi; \
	if lsof -nP -iTCP:$(RENODE_GDB_PORT) -sTCP:LISTEN >/dev/null 2>&1; then \
		echo "Port $(RENODE_GDB_PORT) is already in use -- killing existing Renode session" >&2; \
		kill $$(lsof -nP -tiTCP:$(RENODE_GDB_PORT) -sTCP:LISTEN) 2>/dev/null; \
		while lsof -nP -iTCP:$(RENODE_GDB_PORT) -sTCP:LISTEN >/dev/null 2>&1; do sleep 0.2; done; \
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
