#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "softfloat.h"

extern float32_t f32_add(float32_t, float32_t);
extern float32_t f32_sub(float32_t, float32_t);

extern uint_fast8_t softfloat_exceptionFlags;
extern uint_fast8_t softfloat_roundingMode;
extern uint8_t softfloat_detectTininess;
extern int_fast8_t softfloat_countLeadingZeros8(uint8_t);

#ifdef __cplusplus
}
#endif






#define MEM_SIZE (1 << 24)
#define OPCODE_LUI 0b0110111
#define OPCODE_AUIPC 0b0010111
#define OPCODE_JAL 0b1101111
#define OPCODE_JALR 0b1100111
#define OPCODE_BR 0b1100011
#define FUNCT3_BEQ 0b000
#define FUNCT3_BNE 0b001
#define FUNCT3_BLT 0b100
#define FUNCT3_BGE 0b101
#define FUNCT3_BLTU 0b110
#define FUNCT3_BGEU 0b111
#define OPCODE_LOAD 0b0000011
#define FUNCT3_LB 0b000
#define FUNCT3_LH 0b001
#define FUNCT3_LW 0b010
#define FUNCT3_LBU 0b100
#define FUNCT3_LHU 0b101
#define OPCODE_STORE 0b0100011
#define FUNCT3_SB 0b000
#define FUNCT3_SH 0b001
#define FUNCT3_SW 0b010
#define OPCODE_OPI 0b0010011
#define FUNCT3_ADDI 0b000
#define FUNCT3_SLTI 0b010
#define FUNCT3_SLTIU 0b011
#define FUNCT3_XORI 0b100
#define FUNCT3_ORI 0b110
#define FUNCT3_ANDI 0b111
#define FUNCT3_SLLI 0b001
#define FUNCT3_SRLI 0b101
#define FUNCT7_SLLI 0b0000000
#define FUNCT7_SRLI 0b0000000
#define FUNCT7_SRAI 0b0100000
#define OPCODE_OP 0b0110011
#define FUNCT3_ADD 0b000
#define FUNCT3_SLL 0b001
#define FUNCT3_SLT 0b010
#define FUNCT3_SLTU 0b011
#define FUNCT3_XOR 0b100
#define FUNCT3_SRL 0b101
#define FUNCT3_OR 0b110
#define FUNCT3_AND 0b111
#define FUNCT7_ADD 0b0000000
#define FUNCT7_SUB 0b0100000
#define FUNCT7_SLL 0b0000000
#define FUNCT7_SLT 0b0000000
#define FUNCT7_SLTU 0b0000000
#define FUNCT7_XOR 0b0000000
#define FUNCT7_SRL 0b0000000
#define FUNCT7_SRA 0b0100000
#define FUNCT7_OR 0b0000000
#define FUNCT7_AND 0b0000000
#define OPCODE_FENCE 0b0001111
#define OPCODE_ECALL 0b1110011
#define REGISTER_SP 2
 /********************************************/
#define OPCODE_LOAD_FP 0b0000111
#define OPCODE_OP_FP 0b1010011
#define OPCODE_STORE_FP 0b0100111


 /****************************************** */
struct elf32_header {
    uint8_t e_ident[16];
    uint16_t e_type, e_machine;
    uint32_t e_version, e_entry, e_phoff, e_shoff, e_flags;
    uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
};

struct elf32_phdr {
    uint32_t p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
};

int cpu_run(uint32_t init_pc, uint32_t instr_mem[MEM_SIZE / 4], uint8_t mem0[MEM_SIZE / 4], uint8_t mem1[MEM_SIZE / 4],
            uint8_t mem2[MEM_SIZE / 4], uint8_t mem3[MEM_SIZE / 4]);

void print_bin(uint32_t x) {
    for (uint32_t i = 31; (i + 1) != 0; --i) {
        if (x & (1 << i))
            printf("1");
        else
            printf("0");
    }
}
uint32_t g_instr_mem[MEM_SIZE / 4] = {0};
uint8_t g_mem0[MEM_SIZE / 4] = {0};
uint8_t g_mem1[MEM_SIZE / 4] = {0};
uint8_t g_mem2[MEM_SIZE / 4] = {0};
uint8_t g_mem3[MEM_SIZE / 4] = {0};

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s [executable].\n", argv[0]);
        return 1;
    }
    // map the input file into memory as an elf32 header
    int fd;
    if ((fd = open(argv[1], O_RDONLY)) < 0) {
        printf("Error opening %s\n", argv[1]);
        return 1;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        printf("Error during fstat.\n");
        return 1;
    }
    uint64_t fileSize = st.st_size;
    struct elf32_header *elfHeader;
    if ((elfHeader = (struct elf32_header *)mmap(0, fileSize, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        printf("Error during mmap.\n");
        return 1;
    }

    // iterate over elf32 program headers to load program in memory
    char *current = ((char *)elfHeader) + elfHeader->e_phoff;
    for (int index = 0; index < elfHeader->e_phnum; ++index) {
        struct elf32_phdr *programHeader = (struct elf32_phdr *)current;
        if (programHeader->p_type == 1) {
            char *segment = ((char *)elfHeader) + programHeader->p_offset;
            uint32_t addr = programHeader->p_vaddr;
            uint32_t base_addr = programHeader->p_vaddr / 4;
            uint32_t addr_off = 0;
            uint32_t index2;
            for (index2 = 0; index2 < programHeader->p_filesz; ++index2) {
                switch (addr_off) {
                case 0:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0xFFFFFF00) | (*segment);
                    g_mem0[base_addr] = *segment;
                    break;
                case 1:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0xFFFF00FF) | ((*segment) << 8);
                    g_mem1[base_addr] = *segment;
                    break;
                case 2:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0xFF00FFFF) | ((*segment) << 16);
                    g_mem2[base_addr] = *segment;
                    break;
                default:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0x00FFFFFF) | ((*segment) << 24);
                    g_mem3[base_addr] = *segment;
                    break;
                }
                if (addr_off == 3) {
                    addr_off = 0;
                    addr += 4;
                    ++base_addr;
                } else {
                    ++addr_off;
                }
                ++segment;
            }
            for (; index2 < programHeader->p_memsz; ++index2) {
                switch (addr_off) {
                case 0:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0xFFFFFF00);
                    g_mem0[base_addr] = 0;
                    break;
                case 1:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0xFFFF00FF);
                    g_mem1[base_addr] = 0;
                    break;
                case 2:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0xFF00FFFF);
                    g_mem2[base_addr] = 0;
                    break;
                default:
                    g_instr_mem[addr] = (g_instr_mem[addr] & 0x00FFFFFF);
                    g_mem3[base_addr] = 0;
                    break;
                }
                if (addr_off == 3) {
                    addr_off = 0;
                    addr += 4;
                    ++base_addr;
                } else {
                    ++addr_off;
                }
            }
        }
        current += elfHeader->e_phentsize;
    }
#ifndef NOTRACE
    printf("init_pc = 0x%x\n", elfHeader->e_entry);
#endif
    cpu_run(elfHeader->e_entry, g_instr_mem, g_mem0, g_mem1, g_mem2, g_mem3);

    munmap(elfHeader, fileSize);
    close(fd);
    return 0;
}



#pragma SHLS toplevel
int cpu_run(uint32_t init_pc, uint32_t instr_mem[MEM_SIZE / 4], uint8_t mem0[MEM_SIZE / 4], uint8_t mem1[MEM_SIZE / 4],
            uint8_t mem2[MEM_SIZE / 4], uint8_t mem3[MEM_SIZE / 4]) {
    #pragma HLS interface mode=bram port=instr_mem
    #pragma HLS interface mode=bram port=mem0
    #pragma HLS interface mode=bram port=mem1
    #pragma HLS interface mode=bram port=mem2
    #pragma HLS interface mode=bram port=mem3

    /*******************************************/
    #pragma HLS interface mode=bram port=f
    #pragma HLS interface ap_none port=fflags
    #pragma HLS interface ap_none port=frm
    /****************************************** */

    uint32_t pc = init_pc;



    /**********************************************/
    
    float f[32];
    uint32_t fcsr = 0; 
    uint32_t fflags = 0;
    uint8_t frm = (fcsr >> 5) & 0b111;

    /****************************************** */


    uint32_t x[32] = {0};
    x[REGISTER_SP] = MEM_SIZE - 4;

    bool run = true;
    while (run) {
        uint32_t instr = instr_mem[pc < (MEM_SIZE / 4) ? pc : 0];

        uint8_t opcode = instr & 0b1111111;
        uint8_t rd = (instr >> 7) & 0b11111;
        uint8_t funct3 = (instr >> 12) & 0b111;
        uint8_t rs1 = (instr >> 15) & 0b11111;
        uint8_t rs2 = (instr >> 20) & 0b11111;
        uint8_t funct7 = instr >> 25;
        uint32_t immI = instr >> 20;
        uint32_t simmI = ((instr & 0x80000000u) ? 0xFFFFF000u : 0) | immI;
        uint32_t immS = ((instr >> 7) & 0b11111) | ((instr >> 20) & 0b111111100000);
        uint32_t simmS = ((instr & 0x80000000u) ? 0xFFFFF000u : 0) | immS;
        uint32_t immU = instr & 0xFFFFF000u;
        uint32_t immJ = ((instr >> 20) & 0b11111111110) | ((instr >> 9) & 0b100000000000) |
                        (instr & 0b11111111000000000000) | ((instr >> 11) & 0b100000000000000000000);

        uint32_t simmJ = ((instr & 0x80000000u) ? 0xFFF00000u : 0) | immJ;
        uint32_t immB = ((instr >> 7) & 0b11110) | ((instr >> 20) & 0b11111100000) | ((instr << 4) & 0b100000000000) |
                        ((instr >> 19) & 0b1000000000000);
        uint32_t simmB = ((instr & 0x80000000u) ? 0xFFFFF000u : 0) | immB;
        uint8_t shamt = (instr >> 20) & 0b11111;
#ifndef NOTRACE
        printf("pc=0x%x; ", pc);
#endif
        printf("TRACE: pc=0x%x ; instr=0x%x ; opcode=0x%x\n", pc, instr, opcode);
        switch (opcode) {
        case OPCODE_LUI: {
#ifndef NOTRACE
            printf("lui %d, 0x%x\n", rd, immU);
#endif
            if (rd != 0)
                x[rd] = immU;
            pc += 4;
            break;
        }
        case OPCODE_AUIPC: {
#ifndef NOTRACE
            printf("auipc %d, 0x%x\n", rd, immU);
#endif
            if (rd != 0)
                x[rd] = pc + immU;
            pc += 4;
            break;
        }
        case OPCODE_OPI: {
            switch (funct3) {
            case FUNCT3_ADDI: {
#ifndef NOTRACE
                printf("addi %d, %d, 0x%x; x[rs1]=%d\n", rd, rs1, immI, x[rs1]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] + simmI;
                pc += 4;
                break;
            }
            case FUNCT3_SLTI: {
#ifndef NOTRACE
                printf("slti %d, %d, 0x%x; x[rs1]=%d\n", rd, rs1, immI, x[rs1]);
#endif
                if (rd != 0)
                    x[rd] = ((int32_t)x[rs1]) < ((int32_t)simmI);
                pc += 4;
                break;
            }
            case FUNCT3_SLTIU: {
#ifndef NOTRACE
                printf("sltiu %d, %d, 0x%x; x[rs1]=%d\n", rd, rs1, immI, x[rs1]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] < simmI;
                pc += 4;
                break;
            }
            case FUNCT3_XORI: {
#ifndef NOTRACE
                printf("xori %d, %d, 0x%x; x[rs1]=%d\n", rd, rs1, immI, x[rs1]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] ^ simmI;
                pc += 4;
                break;
            }
            case FUNCT3_ORI: {
#ifndef NOTRACE
                printf("ori %d, %d, 0x%x; x[rs1]=%d\n", rd, rs1, immI, x[rs1]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] | simmI;
                pc += 4;
                break;
            }
            case FUNCT3_ANDI: {
#ifndef NOTRACE
                printf("andi %d, %d, 0x%x; x[rs1]=%d\n", rd, rs1, immI, x[rs1]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] & simmI;
                pc += 4;
                break;
            }
            case FUNCT3_SLLI: {
#ifndef NOTRACE
                printf("slli %d, %d, %d; x[rs1]=%d\n", rd, rs1, shamt, x[rs1]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] << shamt;
                pc += 4;
                break;
            }
            case FUNCT3_SRLI: {
                switch (funct7) {
                case FUNCT7_SRLI: {
#ifndef NOTRACE
                    printf("srli %d, %d, %d; x[rs1]=%d\n", rd, rs1, shamt, x[rs1]);
#endif
                    if (rd != 0)
                        x[rd] = x[rs1] >> shamt;
                    pc += 4;
                    break;
                }
                case FUNCT7_SRAI: {
#ifndef NOTRACE
                    printf("srai %d, %d, %d; x[rs1]=%d\n", rd, rs1, shamt, x[rs1]);
#endif
                    if (rd != 0)
                        x[rd] = ((int32_t)x[rs1]) >> ((int32_t)shamt);
                    pc += 4;
                    break;
                }
                }
                break;
            }
            }
            break;
        }
        case OPCODE_OP: {
            switch (funct3) {
            case FUNCT3_ADD: {
                switch (funct7) {
                case FUNCT7_ADD: {
#ifndef NOTRACE
                    printf("add %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                    if (rd != 0)
                        x[rd] = x[rs1] + x[rs2];
                    pc += 4;
                    break;
                }
                case FUNCT7_SUB: {
#ifndef NOTRACE
                    printf("sub %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                    if (rd != 0)
                        x[rd] = x[rs1] - x[rs2];
                    pc += 4;
                    break;
                }
                }
                break;
            }
            case FUNCT3_SLL: {
#ifndef NOTRACE
                printf("sll %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] << x[rs2];
                pc += 4;
                break;
            }
            case FUNCT3_SLT: {
#ifndef NOTRACE
                printf("slt %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                if (rd != 0)
                    x[rd] = ((int32_t)x[rs1]) < ((int32_t)x[rs2]);
                pc += 4;
                break;
            }
            case FUNCT3_SLTU: {
#ifndef NOTRACE
                printf("sltu %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] < x[rs2];
                pc += 4;
                break;
            }
            case FUNCT3_XOR: {
#ifndef NOTRACE
                printf("xor %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] ^ x[rs2];
                pc += 4;
                break;
            }
            case FUNCT3_SRL: {
                switch (funct7) {
                case FUNCT7_SRL: {
#ifndef NOTRACE
                    printf("srl %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                    if (rd != 0)
                        x[rd] = x[rs1] >> x[rs2];
                    pc += 4;
                    break;
                }
                case FUNCT7_SRA: {
#ifndef NOTRACE
                    printf("sra %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                    if (rd != 0)
                        x[rd] = ((int32_t)x[rs1]) >> ((int32_t)x[rs2]);
                    pc += 4;
                    break;
                }
                }
                break;
            }
            case FUNCT3_OR: {
#ifndef NOTRACE
                printf("or %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] | x[rs2];
                pc += 4;
                break;
            }
            case FUNCT3_AND: {
#ifndef NOTRACE
                printf("and %d, %d, %d; x[rs1]=%d; x[rs2]=%d\n", rd, rs1, rs2, x[rs1], x[rs2]);
#endif
                if (rd != 0)
                    x[rd] = x[rs1] & x[rs2];
                pc += 4;
                break;
            }
            }
            break;
        }
        case OPCODE_FENCE: {
#ifndef NOTRACE
            printf("fence\n");
#endif
            pc += 4;
            break;
        }
        case OPCODE_ECALL: {
#ifndef NOTRACE
            printf("ecall [handled as exit].\n");
#endif
            run = false;
            break;
        }
        case OPCODE_LOAD: {
            switch (funct3) {
            case FUNCT3_LB:
            case FUNCT3_LBU: {
                uint32_t rd_val = 0;
                uint32_t addr = x[rs1] + simmI;
                uint32_t base_addr = addr >> 2;
                uint32_t offset_addr = addr & 0b11;
                switch (offset_addr) {
                case 0:
                    rd_val = (uint32_t)mem0[base_addr < (MEM_SIZE / 4) ? base_addr : 0];
                    break;
                case 1:
                    rd_val = (uint32_t)mem1[base_addr < (MEM_SIZE / 4) ? base_addr : 0];
                    break;
                case 2:
                    rd_val = (uint32_t)mem2[base_addr < (MEM_SIZE / 4) ? base_addr : 0];
                    break;
                default:
                    rd_val = (uint32_t)mem3[base_addr < (MEM_SIZE / 4) ? base_addr : 0];
                    break;
                }
                if (funct3 == FUNCT3_LB) {
#ifndef NOTRACE
                    printf("lb %d, 0x%x(%d); x[rs1]=%d\n", rd, immI, rs1, x[rs1]);
#endif
                    if (rd != 0)
                        x[rd] = ((rd_val & 0b10000000) ? 0xFFFFFF00u : 0) | rd_val;
                } else {
#ifndef NOTRACE
                    printf("lbu %d, 0x%x(%d); x[rs1]=%d\n", rd, immI, rs1, x[rs1]);
#endif
                    if (rd != 0)
                        x[rd] = rd_val;
                }
                pc += 4;
                break;
            }
            case FUNCT3_LH:
            case FUNCT3_LHU: {
                uint32_t rd_val = 0;
                uint32_t addr = x[rs1] + simmI;
                uint32_t base_addr = addr >> 2;
                uint32_t offset_addr = addr & 0b11;
                switch (offset_addr) {
                case 0:
                    rd_val = ((uint32_t)mem0[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) |
                             (((uint32_t)mem1[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) << 8);
                    break;
                case 2:
                    rd_val = ((uint32_t)mem2[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) |
                             (((uint32_t)mem3[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) << 8);
                    break;
                default:
                    printf("Error: Unaligned memory access.\n");
                    run = false;
                    break;
                }
                if (funct3 == FUNCT3_LH) {
#ifndef NOTRACE
                    printf("lh %d, 0x%x(%d); x[rs1]=%d\n", rd, immI, rs1, x[rs1]);
#endif
                    if (rd != 0)
                        x[rd] = ((rd_val & 0x8000) ? 0xFFFF0000u : 0) | rd_val;
                } else {
#ifndef NOTRACE
                    printf("lhu %d, 0x%x(%d); x[rs1]=%d\n", rd, immI, rs1, x[rs1]);
#endif
                    if (rd != 0)
                        x[rd] = rd_val;
                }

                pc += 4;
                break;
            }
            case FUNCT3_LW: {
#ifndef NOTRACE
                printf("lw %d, 0x%x(%d); x[rs1]=%d\n", rd, immI, rs1, x[rs1]);
#endif
                uint32_t rd_val = 0;
                uint32_t addr = x[rs1] + simmI;
                uint32_t base_addr = addr >> 2;
                uint32_t offset_addr = addr & 0b11;
                if (offset_addr == 0) {
                    rd_val = ((uint32_t)mem0[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) |
                             (((uint32_t)mem1[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) << 8) |
                             (((uint32_t)mem2[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) << 16) |
                             (((uint32_t)mem3[base_addr < (MEM_SIZE / 4) ? base_addr : 0]) << 24);
                } else {
#ifndef NOTRACE
                    printf("Error: Unaligned memory access.\n");
#endif
                    run = false;
                }
                if (rd != 0)
                    x[rd] = rd_val;
                pc += 4;
                break;
            }
            }
            break;
        }
        case OPCODE_STORE: {
            switch (funct3) {
            case FUNCT3_SB: {
#ifndef NOTRACE
                printf("sb %d, 0x%x(%d); x[rs1]=%d\n", rs2, immS, rs1, x[rs1]);
#endif
                uint32_t addr = x[rs1] + simmS;
                uint32_t base_addr = addr >> 2;
                uint32_t offset_addr = addr & 0b11;
                uint8_t write_val = x[rs2] & 0b11111111;
                switch (offset_addr) {
                case 0:
                    mem0[base_addr] = write_val;
                    break;
                case 1:
                    mem1[base_addr] = write_val;
                    break;
                case 2:
                    mem2[base_addr] = write_val;
                    break;
                default:
                    mem3[base_addr] = write_val;
                    break;
                }
                pc += 4;
                break;
            }
            case FUNCT3_SH: {
#ifndef NOTRACE
                printf("sh %d, 0x%x(%d); x[rs1]=%d; x[rs2]=%d\n", rs2, immS, rs1, x[rs1], x[rs2]);
#endif
                uint32_t addr = x[rs1] + simmS;
                uint32_t base_addr = addr >> 2;
                uint32_t offset_addr = addr & 0b11;
                uint8_t write_val1 = x[rs2] & 0b11111111;
                uint8_t write_val2 = (x[rs2] >> 8) & 0b11111111;
                switch (offset_addr) {
                case 0:
                    mem0[base_addr] = write_val1;
                    mem1[base_addr] = write_val2;
                    break;
                case 2:
                    mem2[base_addr] = write_val1;
                    mem3[base_addr] = write_val2;
                    break;
                default:
                    printf("Error: Unaligned memory access.\n");
                    run = false;
                }
                pc += 4;
                break;
            }
            case FUNCT3_SW: {
#ifndef NOTRACE
                printf("sw %d, 0x%x(%d); x[rs1]=%d; x[rs2]=%d\n", rs2, immS, rs1, x[rs1], x[rs2]);
#endif
                uint32_t addr = x[rs1] + simmS;
                uint32_t base_addr = addr >> 2;
                uint32_t offset_addr = addr & 0b11;
                uint8_t write_val1 = x[rs2] & 0b11111111;
                uint8_t write_val2 = (x[rs2] >> 8) & 0b11111111;
                uint8_t write_val3 = (x[rs2] >> 16) & 0b11111111;
                uint8_t write_val4 = (x[rs2] >> 24) & 0b11111111;
                if (offset_addr == 0) {
                    mem0[base_addr] = write_val1;
                    mem1[base_addr] = write_val2;
                    mem2[base_addr] = write_val3;
                    mem3[base_addr] = write_val4;
                } else {
                    printf("Error: Unaligned memory access.\n");
                    run = false;
                }
                pc += 4;
                break;
            }
            }
            break;
        }
        case OPCODE_JAL: {
#ifndef NOTRACE
            printf("jal %d, 0x%x\n", rd, immJ);
#endif
            if (rd != 0)
                x[rd] = pc + 4;
            pc += simmJ;
            break;
        }
        case OPCODE_JALR: {
#ifndef NOTRACE
            printf("jalr %d, %d, 0x%x; x[rs1]=%d\n", rd, rs1, immI, x[rs1]);
#endif
            uint32_t t = pc + 4;
            pc = x[rs1] + simmI;
            if (rd != 0)
                x[rd] = t;
            break;
        }
        case OPCODE_BR: {
            switch (funct3) {
            case FUNCT3_BEQ: {
#ifndef NOTRACE
                printf("beq %d, %d, 0x%x; x[rs1]=%d; x[rs2]=%d\n", rs1, rs2, immB, x[rs1], x[rs2]);
#endif
                if (x[rs1] == x[rs2])
                    pc += simmB;
                else
                    pc += 4;
                break;
            }
            case FUNCT3_BNE: {
#ifndef NOTRACE
                printf("bne %d, %d, 0x%x; x[rs1]=%d; x[rs2]=%d\n", rs1, rs2, immB, x[rs1], x[rs2]);
#endif
                if (x[rs1] == x[rs2])
                    pc += 4;
                else
                    pc += simmB;
                break;
            }
            case FUNCT3_BLT: {
#ifndef NOTRACE
                printf("blt %d, %d, 0x%x; x[rs1]=%d; x[rs2]=%d\n", rs1, rs2, immB, x[rs1], x[rs2]);
#endif
                if (((int32_t)x[rs1]) < ((int32_t)x[rs2]))
                    pc += simmB;
                else
                    pc += 4;
                break;
            }
            case FUNCT3_BGE: {
#ifndef NOTRACE
                printf("bge %d, %d, 0x%x; x[rs1]=%d; x[rs2]=%d\n", rs1, rs2, immB, x[rs1], x[rs2]);
#endif
                if (((int32_t)x[rs1]) >= ((int32_t)x[rs2]))
                    pc += simmB;
                else
                    pc += 4;
                break;
            }
            case FUNCT3_BLTU: {
#ifndef NOTRACE
                printf("bltu %d, %d, 0x%x; x[rs1]=%d; x[rs2]=%d\n", rs1, rs2, immB, x[rs1], x[rs2]);
#endif
                if (x[rs1] < x[rs2])
                    pc += simmB;
                else
                    pc += 4;
                break;
            }
            case FUNCT3_BGEU: {
#ifndef NOTRACE
                printf("bgeu %d, %d, 0x%x; x[rs1]=%d; x[rs2]=%d\n", rs1, rs2, immB, x[rs1], x[rs2]);
#endif
                if (x[rs1] >= x[rs2])
                    pc += simmB;
                else
                    pc += 4;
                break;
            }
            }
            break;
        }
        /******************************************************************************/

            // Traitement de l’instruction flw

        case OPCODE_LOAD_FP: {
            if (funct3 == 0b010) {
                uint32_t addr = x[rs1] + simmI;
                uint32_t base = addr >> 2;   
                printf(">>> x[10]=0x%x ; x[11]=0x%x ; x[3]=0x%x ; pc=0x%x\n", x[10], x[11], x[3], pc);

                printf("DEBUG: Avant flw, x[rs1]=%u (0x%x), simmI=%u (0x%x), addr=0x%x, base=%u\n",
                    x[rs1], x[rs1], simmI, simmI, addr, base);
        
                if ((addr & 0x3) == 0) {
                    if (base >= (MEM_SIZE / 4)) {
                        printf("Segmentation fault: invalid memory access 0x%x (base_addr=%u) at pc=0x%x\n", addr, base, pc);
                        run = false;
                        break;
                    }
                    uint32_t val = mem0[base] | (mem1[base] << 8)
                                 | (mem2[base] << 16) | (mem3[base] << 24);
        
                    memcpy(&f[rd], &val, sizeof(float));
                }
                #ifndef NOTRACE
                 printf("flw %d, %d(%d); f[%d] = %f\n", rd, simmI, rs1, rd, f[rd]);
                #endif
        
                pc += 4;
            }
        
            break;
        }



                    // Traitement de l’instruction fsw

                    case OPCODE_STORE_FP: {
                        if (funct3 == 0b010) {
                            uint32_t addr = x[rs1] + simmS;
                            uint32_t base = addr >> 2;
                    
                            uint32_t val;
                            memcpy(&val, &f[rs2], sizeof(float));
                    
                            mem0[base] = val & 0xFF;
                            mem1[base] = (val >> 8) & 0xFF;
                            mem2[base] = (val >> 16) & 0xFF;
                            mem3[base] = (val >> 24) & 0xFF;
                    
                    #ifndef NOTRACE
                            printf("fsw %d, %d(%d); f[%d] = %f\n", rs2, simmS, rs1, rs2, f[rs2]);
                    #endif
                    
                            pc += 4;
                        }
                        break;
                    }
                    

                        
        case OPCODE_OP_FP: {
            printf("OPCODE_OP_FP: instr=0x%x pc=0x%x funct7=0x%x funct3=0x%x rd=%d rs1=%d rs2=%d\n",
                instr, pc, funct7, funct3, rd, rs1, rs2);
            unsigned char frm = (fcsr >> 5) & 0b111;
            unsigned char mode;
        
            // Choix du mode d’arrondi en fonction de frm
            switch (frm) {
                case 0: mode = softfloat_round_near_even; break;
                case 1: mode = softfloat_round_minMag;    break;
                case 2: mode = softfloat_round_min;       break;
                case 3: mode = softfloat_round_max;       break;
                case 4: mode = softfloat_round_near_maxMag; break;
                default: mode = softfloat_round_near_even;
            }
                    softfloat_roundingMode = mode;
        
            // Traitement de l’instruction fadd.s
            if (funct7 == 0b0000000 && (funct3 == 0b000 || funct3 == 0b111)){
                float32_t a = { .v = *(uint32_t*)&f[rs1] };
                float32_t b = { .v = *(uint32_t*)&f[rs2] };
        
                softfloat_exceptionFlags = 0;
        
                float32_t result = f32_add(a, b);
        
                if (rd != 0) {
                    f[rd] = *(float*)&result.v;
                }
        
                fcsr = (fcsr & ~0x1F) | (softfloat_exceptionFlags & 0x1F);
        
        #ifndef NOTRACE
                printf("Instruction : fadd.s f%d, f%d, f%d\n", rd, rs1, rs2);
                printf("Résultat    : %f\n", f[rd]);
                printf("fflags      : 0x%02X\n", fcsr & 0x1F);
                if (fcsr & 0x01) printf(" - NV (operation non valide)\n");
                if (fcsr & 0x02) printf(" - DZ (diviser par zero)\n");
                if (fcsr & 0x04) printf(" - OF (plus grand)\n");
                if (fcsr & 0x08) printf(" - UF (plus petit)\n");
                if (fcsr & 0x10) printf(" - NX (inexact / arrondi)\n");
        #endif
                pc += 4;
            }
        
            // Traitement de l’instruction fsub.s
            else if (funct7 == 0b0000100 && (funct3 == 0b000 || funct3 == 0b111)) {
                float32_t a = { .v = *(uint32_t*)&f[rs1] };
                float32_t b = { .v = *(uint32_t*)&f[rs2] };
        
                softfloat_exceptionFlags = 0;
                float32_t result = f32_sub(a, b);
                printf("TRACE: fsub.s va être exécuté avec rs1 = %d, rs2 = %d, rd = %d\n", rs1, rs2, rd);

                if (rd != 0) {
                    f[rd] = *(float*)&result.v;
                }
        
                fcsr = (fcsr & ~0x1F) | (softfloat_exceptionFlags & 0x1F);
        
        #ifndef NOTRACE
                printf("Instruction : fsub.s f%d, f%d, f%d\n", rd, rs1, rs2);
                printf("Résultat    : %f\n", f[rd]);
                printf("fflags      : 0x%02X\n", fcsr & 0x1F);
                if (fcsr & 0x01) printf(" - NV (invalid operation)\n");
                if (fcsr & 0x02) printf(" - DZ (division by zero)\n");
                if (fcsr & 0x04) printf(" - OF (overflow)\n");
                if (fcsr & 0x08) printf(" - UF (underflow)\n");
                if (fcsr & 0x10) printf(" - NX (inexact / arrondi)\n");
        #endif
                pc += 4;
            }
            else {
                #ifndef NOTRACE
                                printf("Instruction OP_FP non reconnue : funct7=0x%x, funct3=0x%x\n", funct7, funct3);
                #endif
                               run = false; 
                            }
                
                            break;
                        }
        



        /*********************************************/

        default: {
#ifndef NOTRACE 
            printf("Error: undefined instruction.\n");
#endif
            run = false;
        }
        }
    }
    return pc;

    
}



