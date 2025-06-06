# Makefile for RISC-V mini simulator

SDKROOT := $(shell xcrun --sdk macosx --show-sdk-path)

CXX ?= clang++
CC ?= clang
CXXFLAGS ?= -std=c++17 -O2 -Wall -isysroot $(SDKROOT) -Isoftfloat
CFLAGS ?= -O2 -Wall -Isoftfloat

# RISC-V toolchain
ARCH       ?= rv32i
ABI        ?= ilp32
RISCV_GCC  ?= riscv64-unknown-elf-gcc
RISCV_FLAGS ?= -march=$(ARCH) -mabi=$(ABI) -nostartfiles -Ttext=0x0

# Simulator executable
SIM ?= riscv_sim

# Liste des objets SoftFloat
SOFTFLOAT_OBJS = \
    softfloat/f32_add.o \
    softfloat/f32_sub.o \
    softfloat/softfloat_raiseFlags.o \
    softfloat/s_addMagsF32.o \
    softfloat/s_subMagsF32.o \
    softfloat/s_roundPackToF32.o \
    softfloat/s_normRoundPackToF32.o \
    softfloat/s_shiftRightJam32.o \
    softfloat/s_countLeadingZeros32.o \
    softfloat/s_propagateNaNF32UI.o

# Default target: build simulator
all: $(SIM)

# Compile the mini-simulator
$(SIM): riscv_sim.cpp softfloat_link.o $(SOFTFLOAT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ riscv_sim.cpp softfloat_link.o $(SOFTFLOAT_OBJS)

# Compilation des .c de SoftFloat en .o
softfloat/%.o: softfloat/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compilation du linker
softfloat_link.o: softfloat_link.c
	$(CC) $(CFLAGS) -c $< -o $@

softfloat/f32_add_sub_global.o: softfloat/f32_add_sub_global.c
	$(CC) $(CFLAGS) -c $< -o $@

%.elf: %.S
	$(RISCV_GCC) $(RISCV_FLAGS) -o $@ $<

# Generic run (set PROG=name of .elf file without extension)
run: $(SIM) $(PROG).elf
	./$(SIM) $(PROG).elf

run-fp: ARCH=rv32if
run-fp: ABI=ilp32f
run-fp: run

# Clean generated files
clean:
	rm -f $(SIM) *.elf *.o trace.txt

.PHONY: all run run-fp clean
