//
// Created by Ricardo Jacobi on 18/11/22.
//

#include "globals.h"
#include "riscv.h"
#include "memoria.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

//
// Initial values for registers
//

void init() {
    pc = 0;
    ri = 0;
    stop_prg = 0;
    sp = 0x3ffc;
    gp = 0x1800;
    build_dic();
}

void fetch() {
    ri = lw(pc, 0);
    pc = pc + 4;
}

void decode() {
    int32_t tmp;

    opcode = ri & 0x7F;
    rs2 = (ri >> 20) & 0x1F;
    rs1 = (ri >> 15) & 0x1F;
    rd = (ri >> 7) & 0x1F;
    shamt = (ri >> 20) & 0x1F;
    funct3 = (ri >> 12) & 0x7;
    funct7 = (ri >> 25);
    imm12_i = ((int32_t) ri) >> 20;
    tmp = get_field(ri, 7, 0x1f);
    imm12_s = set_field(imm12_i, 0, 0x1f, tmp);
    imm13 = imm12_s;
    imm13 = set_bit(imm13, 11, imm12_s & 1);
    imm13 = imm13 & ~1;
    imm20_u = ri & (~0xFFF);
// mais aborrecido...
    imm21 = (int32_t) ri >> 11;            // estende sinal
    tmp = get_field(ri, 12, 0xFF);        // le campo 19:12
    imm21 = set_field(imm21, 12, 0xFF, tmp);    // escreve campo em imm21
    tmp = get_bit(ri, 20);                // le o bit 11 em ri(20)
    imm21 = set_bit(imm21, 11, tmp);            // posiciona bit 11
    tmp = get_field(ri, 21, 0x3FF);
    imm21 = set_field(imm21, 1, 0x3FF, tmp);
    imm21 = imm21 & ~1;                    // zero bit 0
    ins_code = get_instr_code(opcode, funct3, funct7);
}

void debug_decode() {
    int32_t tmp = get_imm32(get_i_format(opcode, funct3, funct7));

    printf("Inst = %s\nPC = %x \nimm32 = %d / %x\nrs1 = %s : rs2 = %s : rd = %s\n",
           instr_str[ins_code],
           pc-4,
           tmp, tmp, reg_str[rs1], reg_str[rs2], reg_str[rd]
           );
}

void dump_breg() {
    for (int i = 0; i < 32; i++) {
        printf("BREG[%s] = \t%8d \t\t\t%8x\n", instr_str[i], breg[i], breg[i]);
    }
}

void dump_asm(int start_ins, int end_ins) {
    struct instruction_context_st ic;

    for (int i = start_ins; i <= end_ins; i++) {
        fetch();
        //cout  << "mem[" << i << "] =  " << x << endl;
        decode();
        debug_decode();
    }
}

void dump_mem(int start_byte, int end_byte, char format) {
    switch (format) {
        case 'h':
        case 'H':
            for (uint32_t i = start_byte; i <= end_byte; i += 4)
                printf("%08x \t%08x\n", i, lw(i, 0));
            break;
        case 'd':
        case 'D':
            for (int i = start_byte; i <= end_byte; i += 4)
                printf("%x \t%d\n", i, lw(i, 0));
            break;
        default:
            break;
    }
}

int load_mem(const char *fn, int start) {
    FILE *fptr;
    int *m_ptr = mem + (start >> 2);
    int size = 0;

    fptr = fopen(fn, "rb");
    if (!fptr) {
        printf("Arquivo nao encontrado!");
        return -1;
    } else {
        while (!feof(fptr)) {
            fread(m_ptr, 4, 1, fptr);
            m_ptr++;
            size++;
        }
        fclose(fptr);
    }
    return size;
}

int32_t get_imm32(enum FORMATS iformat) {
    switch (iformat) {
        case RType:     return 0;
        case IType:     return imm12_i;
        case SType:     return imm12_i;
        case SBType:    return imm13;
        case UType:     return imm20_u;
        case UJType:    return imm21;
        default:        return 0;
    }
}

enum FORMATS get_i_format(uint32_t opcode, uint32_t func3, uint32_t func7) {
    switch (opcode) {
        case 0x33 :
            return RType;
        case 0x03:
        case 0x13:
        case 0x67:
        case 0x73:
            return IType;
        case 0x23 :
            return SType;
        case 0x63 :
            return SBType;
        case 0x37 :
            return UType;
        case 0x6F:
        case 0x17 :
            return UJType;
        case 0x00:
            if (func3 == 0 && func7 == 0)
                return NOPType;
            else
                return NullFormat;
        default:
            printf("Undefined Format");
            return NullFormat;
            break;
    }
}

enum INSTRUCTIONS get_instr_code(uint32_t opcode, uint32_t funct3, uint32_t funct7) {
    switch (opcode) {
        case LUI:
            return I_lui;
        case AUIPC:
            return I_auipc;
        case BType:
            switch (funct3) {
                case BEQ3:
                    return I_beq;
                case BNE3:
                    return I_bne;
                case BLT3:
                    return I_blt;
                case BGE3:
                    return I_bge;
                case BLTU3:
                    return I_bltu;
                case BGEU3:
                    return I_bgeu;
            }
            break;
        case ILType:
            switch (funct3) {
                case LB3:
                    return I_lb;
                case LH3:
                    return I_lh;
                case LW3:
                    return I_lw;
                case LBU3:
                    return I_lbu;
                default:
                    break;
            }
            break;
        case JAL:
            return I_jal;
        case JALR:
            return I_jalr;
        case StoreType:
            switch (funct3) {
                case SB3:
                    return I_sb;
                case SH3:
                    return I_sh;
                case SW3:
                    return I_sw;
                default:
                    break;
            }
            break;
        case ILAType:
            switch (funct3) {
                case ADDI3:
                    return I_addi;
                case ORI3:
                    return I_ori;
                case ANDI3:
                    return I_andi;
                case XORI3:
                    return I_xori;
                case SLTI3:
                    return I_slti;
                case SLTIU3:
                    return I_sltiu;
                case SLLI3:
                    return I_slli;
                case SRI3:
                    if (funct7 == SRLI7) return I_srli;
                    else return I_srai;
                default:
                    break;
            }
            break;
        case RegType:
            switch (funct3) {
                case ADDSUB3:
                    if (funct7 == SUB7) return I_sub;
                    else return I_add;
                case SLL3:
                    return I_sll;
                case SLT3:
                    return I_slt;
                case SLTU3:
                    return I_sltu;
                case XOR3:
                    return I_xor;
                case OR3:
                    return I_or;
                case AND3:
                    return I_and;
                case SR3:
                    if (funct7 == SRA7) return I_sra;
                    else return I_srl;
                default:
                    break;
            }
            break;
        case ECALL:
            return I_ecall;
        default:
            printf("\n\nInstrucao Invalida (PC = %08x RI = %08x)\n", pc, ri);
            break;
    }
    return I_nop;
}

void execute() {
// insert your code here...
}

void step() {
    fetch();
    decode();
    execute();
}

void run() {
    init();
    while ((pc < DATA_SEGMENT_START) && !stop_prg)
        step();
}