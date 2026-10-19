# -------- Toolchain --------
CC      := arm-none-eabi-gcc
GDB     := arm-none-eabi-gdb
READELF := arm-none-eabi-readelf
NM      := arm-none-eabi-nm
OBJDUMP := arm-none-eabi-objdump
RENODE  := renode

# -------- Target --------
CPU   := cortex-m3

# Board to build/debug/flash. stm32f103c8t6 is the real Blue Pill
# hardware; override with BOARD=stm32f103 for the Renode-only high-density
# variant.
BOARD ?= stm32f103c8t6

# -------- Paths --------
BUILD_DIR := build
ELF       := $(BUILD_DIR)/rtos.elf
APP_SRC   ?= app/app_main.c

ifeq ($(BOARD),stm32f103)
LD_SCRIPT            := linker/stm32f103.ld
BSP_SRC               := bsp/stm32f103/systick.c
RENODE_RUN_SCRIPT     := scripts/renode/stm32f103_run.resc
RENODE_DEBUG_SCRIPT   := scripts/renode/stm32f103_debug.resc
else ifeq ($(BOARD),stm32f103c8t6)
LD_SCRIPT            := linker/stm32f103c8t6.ld
BSP_SRC               := bsp/stm32f103/systick.c
RENODE_RUN_SCRIPT     := scripts/renode/stm32f103c8t6_run.resc
RENODE_DEBUG_SCRIPT   := scripts/renode/stm32f103c8t6_debug.resc
else
$(error Unknown BOARD '$(BOARD)' -- expected stm32f103 or stm32f103c8t6)
endif

# -------- Real hardware clock bring-up --------
# HW=1 (default): compiles in bsp/stm32f103/clock.c and runs real
# HSE+PLL bring-up to 72MHz before the scheduler starts ticking --
# correct for the real Blue Pill, which is the default BOARD.
#
# HW=0: boots on the reset-default 8MHz HSI. Forced regardless of any
# override when BOARD=stm32f103 (Renode-only) -- Renode's platform
# files don't model RCC, so an HSERDY/PLLRDY poll there spins forever.
HW ?= 1
ifeq ($(BOARD),stm32f103)
override HW := 0
endif

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
	kernel/rtt.c \
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

.PHONY: build check-board bench-sched renode renode-debug debug sections segments symbols disasm clean

# ---------- Build ----------
BOARD_MARKER := $(BUILD_DIR)/.board_marker
BUILD_ID := $(BOARD)|$(APP_SRC)|$(HW)

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
# server to come up, attaches gdb, then kills Renode when gdb exits. 
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

flash:
	STM32_Programmer_CLI -c port=SWD -w build/rtos.elf -rst

# Commands for interacting with board from remote Raspberry Pi

.PHONY: check-pi
check-pi:
	@test -f build/rtos.elf || { echo "build/rtos.elf not found, run 'make' first"; exit 1; }
	@nc -z localhost 3333 || { echo "Nothing on localhost:3333, start the tunnel: ssh -N debugpi"; exit 1; }

.PHONY: flash-pi
flash-pi: check-pi
	@ls -l build/rtos.elf
	arm-none-eabi-gdb -batch build/rtos.elf \
		-ex "target extended-remote localhost:3333" \
		-ex "monitor reset halt" \
		-ex "load" \
		-ex "monitor reset run" \
		-ex "detach"

.PHONY: debug-pi
debug-pi: check-pi
	arm-none-eabi-gdb build/rtos.elf -ex "target extended-remote localhost:3333"

# openocd.service on the pi runs with RTT always enabled (see
# scripts/openocd/stm32f103c8t6_rtt.cfg), listening on port 9090.
.PHONY: view-rtt
view-rtt:
	ssh -o ClearAllForwardings=yes debugpi "echo -e 'rtt stop\rrtt start\r' | nc -q1 localhost 4444"
	@nc -z localhost 9090 || { echo "Nothing on localhost:9090, check OpenOCD is running the RTT cfg on the pi and the tunnel forwards port 9090"; exit 1; }
	nc localhost 9090

# OpenOCD's background RTT poller and an active debug-pi session can race
# over the SWD adapter and wedge the RTT bridge (connections on 9090
# accepted then dropped with zero bytes) -- this flushes it via OpenOCD's
# telnet console. ClearAllForwardings avoids clashing with an already-open
# `ssh -N debugpi` tunnel holding the same LocalForward ports.
.PHONY: restart-rtt-pi
restart-rtt-pi:
	ssh -o ClearAllForwardings=yes debugpi "echo -e 'rtt stop\rrtt start\r' | nc -q1 localhost 4444"


