
#include "rv/cpu.hpp"
#include "rv/memory.hpp"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <string>

namespace rv {

CPU::CPU(Memory& mem) : mem_(mem) {
    reset(0);
}

void CPU::reset(uint32_t start_pc) {
    pc_ = start_pc;
    regs_.fill(0);
    regs_[0] = 0;

    csr_.fill(0);                 // ✅ clear CSRs
         
}

static inline uint32_t get_bits(uint32_t x, int hi, int lo) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1);
}

static inline int32_t sign_extend(uint32_t x, int bits) {
    uint32_t m = 1u << (bits - 1);
    return (int32_t)((x ^ m) - m);
}

static inline int32_t imm_b(uint32_t inst) {
    // B-type: imm[12|10:5|4:1|11] from bits [31|30:25|11:8|7], LSB is 0
    uint32_t imm =
        (get_bits(inst, 31, 31) << 12) |
        (get_bits(inst, 7, 7)   << 11) |
        (get_bits(inst, 30, 25) << 5)  |
        (get_bits(inst, 11, 8)  << 1);
    return sign_extend(imm, 13);
}

static inline int32_t imm_j(uint32_t inst) {
    // J-type: imm[20|10:1|11|19:12] from bits [31|30:21|20|19:12], LSB is 0
    uint32_t imm =
        (get_bits(inst, 31, 31) << 20) |
        (get_bits(inst, 19, 12) << 12) |
        (get_bits(inst, 20, 20) << 11) |
        (get_bits(inst, 30, 21) << 1);
    return sign_extend(imm, 21);
}

void CPU::step() {
    uint32_t inst = mem_.load32(pc_);

    uint32_t opcode = get_bits(inst, 6, 0);
    uint32_t rd     = get_bits(inst, 11, 7);
    uint32_t funct3 = get_bits(inst, 14, 12);
    uint32_t rs1    = get_bits(inst, 19, 15);
    uint32_t rs2    = get_bits(inst, 24, 20);
    uint32_t funct7 = get_bits(inst, 31, 25);

    uint32_t next_pc = pc_ + 4;

    // For trace
    const uint32_t pc_before = pc_;
    const uint32_t inst_before = inst;
    const char* mnemonic = "unknown";
    int wb_reg = -1;
    uint32_t wb_val = 0;
    int32_t imm_for_print = 0; // only used for addi trace

    if (inst == 0x00100073) { // EBREAK
      mnemonic = "ebreak";
     if (trace_) {
    std::cout << "PC=0x" << std::hex << std::setw(8) << std::setfill('0') << pc_before
     << " INST=0x" << std::setw(8) << inst_before
     << " " << mnemonic << std::dec << "\n";
}
     throw std::runtime_error("EBREAK");
}
    else if (inst == 0x00000073) { // ECALL
    // Trap handling intentionally not supported.
    // Treat ECALL as a clean stop, similar to EBREAK.
    mnemonic = "ecall";
    if (trace_) {
        std::cout << "PC=0x" << std::hex << std::setw(8) << std::setfill('0') << pc_before
                  << " INST=0x" << std::setw(8) << inst_before
                  << " " << mnemonic << std::dec << "\n";
    }
    throw std::runtime_error("ECALL");
}
    

   else if (opcode == 0x73 && funct3 != 0x0) {
    // CSR instructions (CSRRW/CSRRS/CSRRC + immediate forms)
    uint32_t csr_addr = get_bits(inst, 31, 20);
    uint32_t old = csr_read(csr_addr);

    bool imm_form = (funct3 & 0x4u) != 0;     // 101/110/111
    uint32_t src  = imm_form ? rs1 : regs_[rs1]; // rs1 is zimm for imm forms

    uint32_t newv = old;
    bool do_write = true;

    switch (funct3) {
        case 0x1: // CSRRW
            mnemonic = "csrrw";
            newv = src;
            break;

        case 0x2: // CSRRS
            mnemonic = "csrrs";
            if (src == 0) do_write = false;
            newv = old | src;
            break;

        case 0x3: // CSRRC
            mnemonic = "csrrc";
            if (src == 0) do_write = false;
            newv = old & ~src;
            break;

        case 0x5: // CSRRWI
            mnemonic = "csrrwi";
            newv = src & 0x1Fu;
            break;

        case 0x6: // CSRRSI
            mnemonic = "csrrsi";
            if ((src & 0x1Fu) == 0) do_write = false;
            newv = old | (src & 0x1Fu);
            break;

        case 0x7: // CSRRCI
            mnemonic = "csrrci";
            if ((src & 0x1Fu) == 0) do_write = false;
            newv = old & ~(src & 0x1Fu);
            break;

       default:
    throw std::runtime_error("ILLEGAL_CSR");
    }

    // rd gets OLD CSR value
    if (rd != 0) {
        regs_[rd] = old;
        wb_reg = (int)rd;
        wb_val = old;
    }

    if (do_write) {
        csr_write(csr_addr, newv);
    }
}


    else if (opcode == 0x0F) {
    // FENCE / FENCE.I  (treat as NOP)
    mnemonic = (funct3 == 0x1) ? "fence.i" : "fence";
    // do nothing, just fall through with next_pc = pc_ + 4
}
   
    else if (opcode == 0x13) {
    // I-type ALU (ADDI, ANDI, ORI, XORI, SLTI, SLTIU, SLLI, SRLI, SRAI)
    int32_t imm = sign_extend(get_bits(inst, 31, 20), 12);
    imm_for_print = imm;

    uint32_t a = regs_[rs1];
    uint32_t val = 0;

    switch (funct3) {
        case 0x0: // ADDI
            mnemonic = "addi";
            val = a + (uint32_t)imm;
            break;

        case 0x7: // ANDI
            mnemonic = "andi";
            val = a & (uint32_t)imm;
            break;

        case 0x6: // ORI
            mnemonic = "ori";
            val = a | (uint32_t)imm;
            break;

        case 0x4: // XORI
            mnemonic = "xori";
            val = a ^ (uint32_t)imm;
            break;

        case 0x2: // SLTI (signed)
            mnemonic = "slti";
            val = ((int32_t)a < imm) ? 1u : 0u;
            break;

        case 0x3: // SLTIU (unsigned)
            mnemonic = "sltiu";
            val = (a < (uint32_t)imm) ? 1u : 0u;
            break;

        case 0x1: { // SLLI (shamt in [24:20], funct7 must be 0)
            if (funct7 != 0x00) throw std::runtime_error("ILLEGAL_SLLI");
            mnemonic = "slli";
            uint32_t shamt = get_bits(inst, 24, 20) & 31u;
            val = a << shamt;
            imm_for_print = (int32_t)shamt; // nicer trace
            break;
        }

        case 0x5: { // SRLI/SRAI
            uint32_t shamt = get_bits(inst, 24, 20) & 31u;
            if (funct7 == 0x00) {
                mnemonic = "srli";
                val = a >> shamt;
            } else if (funct7 == 0x20) {
                mnemonic = "srai";
                val = (uint32_t)((int32_t)a >> shamt);
            } else {
                throw std::runtime_error("ILLEGAL_SRLI_SRAI");
            }
            imm_for_print = (int32_t)shamt; // nicer trace
            break;
        }

        default:
            throw std::runtime_error("ILLEGAL_ITYPE_ALU");
    }

    if (rd != 0) {
        regs_[rd] = val;
        wb_reg = (int)rd;
        wb_val = val;
    }
}

else if (opcode == 0x03) {
    // Loads: LB/LH/LW/LBU/LHU
    int32_t imm = sign_extend(get_bits(inst, 31, 20), 12);
    imm_for_print = imm;
    uint32_t addr = regs_[rs1] + (uint32_t)imm;

    uint32_t val = 0;

    switch (funct3) {
        case 0x0: { // LB
            mnemonic = "lb";
            int8_t b = (int8_t)mem_.load8(addr);
            val = (uint32_t)(int32_t)b; // sign-extend
            break;
        }
        case 0x1: { // LH
            mnemonic = "lh";
            int16_t h = (int16_t)mem_.load16(addr);
            val = (uint32_t)(int32_t)h; // sign-extend
            break;
        }
        case 0x2: { // LW
            mnemonic = "lw";
            if (addr % 4 != 0) throw std::runtime_error("UNALIGNED_LW");
            val = mem_.load32(addr);
            break;
        }
        case 0x4: { // LBU
            mnemonic = "lbu";
            uint8_t b = mem_.load8(addr);
            val = (uint32_t)b; // zero-extend
            break;
        }
        case 0x5: { // LHU
            mnemonic = "lhu";
            uint16_t h = mem_.load16(addr);
            val = (uint32_t)h; // zero-extend
            break;
        }
        default:
            throw std::runtime_error("ILLEGAL_LOAD");
    }

    if (rd != 0) {
        regs_[rd] = val;
        wb_reg = (int)rd;
        wb_val = val;
    }
}

else if (opcode == 0x23) {
    // Stores: SB/SH/SW (S-type)
    uint32_t imm11_5 = get_bits(inst, 31, 25);
    uint32_t imm4_0  = get_bits(inst, 11, 7);
    int32_t imm = sign_extend((imm11_5 << 5) | imm4_0, 12);
    imm_for_print = imm;

    uint32_t addr = regs_[rs1] + (uint32_t)imm;

    switch (funct3) {
        case 0x0: // SB
            mnemonic = "sb";
            mem_.store8(addr, (uint8_t)(regs_[rs2] & 0xFF));
            break;

        case 0x1: // SH
            mnemonic = "sh";
            mem_.store16(addr, (uint16_t)(regs_[rs2] & 0xFFFF));
            break;

        case 0x2: // SW
            mnemonic = "sw";
            if (addr % 4 != 0) throw std::runtime_error("UNALIGNED_SW");
            mem_.store32(addr, regs_[rs2]);
            break;

        default:
            throw std::runtime_error("ILLEGAL_STORE");
    }
}

else if (opcode == 0x6F) {
    // JAL
    mnemonic = "jal";
    int32_t imm = imm_j(inst);
    imm_for_print = imm;

    uint32_t ret = pc_ + 4;
    uint32_t target = pc_ + (uint32_t)imm;

    if (rd != 0) {
        regs_[rd] = ret;
        wb_reg = (int)rd;
        wb_val = ret;
    }

    next_pc = target;
}

else if (opcode == 0x67 && funct3 == 0x0) {
    // JALR
    mnemonic = "jalr";
    int32_t imm = sign_extend(get_bits(inst, 31, 20), 12);
    imm_for_print = imm;

    uint32_t ret = pc_ + 4;
    uint32_t target = (regs_[rs1] + (uint32_t)imm) & ~1u;

    if (rd != 0) {
        regs_[rd] = ret;
        wb_reg = (int)rd;
        wb_val = ret;
    }

    next_pc = target;
}

else if (opcode == 0x37) {
    // LUI
    mnemonic = "lui";
    uint32_t imm = inst & 0xFFFFF000u;   // upper 20 bits << 12
    imm_for_print = (int32_t)imm;

    if (rd != 0) {
        regs_[rd] = imm;
        wb_reg = (int)rd;
        wb_val = imm;
    }
}

else if (opcode == 0x17) {
    // AUIPC
    mnemonic = "auipc";
    uint32_t imm = inst & 0xFFFFF000u;
    imm_for_print = (int32_t)imm;

    uint32_t val = pc_ + imm;
    if (rd != 0) {
        regs_[rd] = val;
        wb_reg = (int)rd;
        wb_val = val;
    }
}

else if (opcode == 0x63) {
    // Branches (B-type)
    int32_t off = imm_b(inst);
    imm_for_print = off;

    bool take = false;

    switch (funct3) {
        case 0x0: // BEQ
            mnemonic = "beq";
            take = (regs_[rs1] == regs_[rs2]);
            break;
        case 0x1: // BNE
            mnemonic = "bne";
            take = (regs_[rs1] != regs_[rs2]);
            break;
        case 0x4: // BLT (signed)
            mnemonic = "blt";
            take = ((int32_t)regs_[rs1] < (int32_t)regs_[rs2]);
            break;
        case 0x5: // BGE (signed)
            mnemonic = "bge";
            take = ((int32_t)regs_[rs1] >= (int32_t)regs_[rs2]);
            break;
        case 0x6: // BLTU (unsigned)
            mnemonic = "bltu";
            take = (regs_[rs1] < regs_[rs2]);
            break;
        case 0x7: // BGEU (unsigned)
            mnemonic = "bgeu";
            take = (regs_[rs1] >= regs_[rs2]);
            break;
        default:
            throw std::runtime_error("ILLEGAL_BRANCH");
    }

    

    if (take) {
        next_pc = pc_ + (uint32_t)off;
    }
}
    else if (opcode == 0x33) {
    // R-type ALU ops
    uint32_t a = regs_[rs1];
    uint32_t b = regs_[rs2];
    uint32_t val = 0;

    switch (funct3) {
        case 0x0: // ADD/SUB
            if (funct7 == 0x00) { mnemonic = "add"; val = a + b; }
            else if (funct7 == 0x20) { mnemonic = "sub"; val = a - b; }
            else throw std::runtime_error("ILLEGAL_RTYPE");
            break;

        case 0x7: // AND
            if (funct7 != 0x00) throw std::runtime_error("ILLEGAL_RTYPE");
            mnemonic = "and"; val = a & b;
            break;

        case 0x6: // OR
            if (funct7 != 0x00) throw std::runtime_error("ILLEGAL_RTYPE");
            mnemonic = "or"; val = a | b;
            break;

        case 0x4: // XOR
            if (funct7 != 0x00) throw std::runtime_error("ILLEGAL_RTYPE");
            mnemonic = "xor"; val = a ^ b;
            break;

        case 0x2: // SLT (signed)
            if (funct7 != 0x00) throw std::runtime_error("ILLEGAL_RTYPE");
            mnemonic = "slt"; val = ((int32_t)a < (int32_t)b) ? 1u : 0u;
            break;

        case 0x3: // SLTU (unsigned)
            if (funct7 != 0x00) throw std::runtime_error("ILLEGAL_RTYPE");
            mnemonic = "sltu"; val = (a < b) ? 1u : 0u;
            break;

        case 0x1: // SLL
            if (funct7 != 0x00) throw std::runtime_error("ILLEGAL_RTYPE");
            mnemonic = "sll"; val = a << (b & 31u);
            break;

        case 0x5: // SRL/SRA
            if (funct7 == 0x00) { mnemonic = "srl"; val = a >> (b & 31u); }
            else if (funct7 == 0x20) { mnemonic = "sra"; val = (uint32_t)((int32_t)a >> (b & 31u)); }
            else throw std::runtime_error("ILLEGAL_RTYPE");
            break;

        default:
            throw std::runtime_error("ILLEGAL_RTYPE");
    }

    if (rd != 0) {
        regs_[rd] = val;
        wb_reg = (int)rd;
        wb_val = val;
    }
}
    else {
        if (trace_) {
            std::cout << "PC=0x" << std::hex << std::setw(8) << std::setfill('0') << pc_before
                      << " INST=0x" << std::setw(8) << inst_before
                      << " illegal"
                      << std::dec << "\n";
        }
        throw std::runtime_error("ILLEGAL");
    }

    regs_[0] = 0;
    pc_ = next_pc;

    // Print trace AFTER execution (so WB values are final)
    if (trace_ && inst != 0x00100073 && inst != 0x00000073) {
        std::cout << "PC=0x" << std::hex << std::setw(8) << std::setfill('0') << pc_before
                  << " INST=0x" << std::setw(8) << inst_before
                  << " " << mnemonic;

        // print operands for our 2 instructions
    if (std::string(mnemonic) == "addi" || std::string(mnemonic) == "andi" ||
         std::string(mnemonic) == "ori"  || std::string(mnemonic) == "xori" ||
         std::string(mnemonic) == "slti" || std::string(mnemonic) == "sltiu"||
         std::string(mnemonic) == "slli" || std::string(mnemonic) == "srli" ||
         std::string(mnemonic) == "srai") {
         std::cout << " x" << rd << ",x" << rs1 << "," << std::dec << imm_for_print;
}
        else if (std::string(mnemonic) == "add" || std::string(mnemonic) == "sub" ||
         std::string(mnemonic) == "and" || std::string(mnemonic) == "or"  ||
         std::string(mnemonic) == "xor" || std::string(mnemonic) == "slt" ||
         std::string(mnemonic) == "sltu"|| std::string(mnemonic) == "sll" ||
         std::string(mnemonic) == "srl" || std::string(mnemonic) == "sra") {
         std::cout << " x" << rd << ",x" << rs1 << ",x" << rs2;
}

       else if (std::string(mnemonic) == "lb" || std::string(mnemonic) == "lh" ||
         std::string(mnemonic) == "lw" || std::string(mnemonic) == "lbu"||
         std::string(mnemonic) == "lhu") {
         std::cout << " x" << rd << "," << std::dec << imm_for_print << "(x" << rs1 << ")";
}
      else if (std::string(mnemonic) == "sb" || std::string(mnemonic) == "sh" ||
         std::string(mnemonic) == "sw") {
         std::cout << " x" << rs2 << "," << std::dec << imm_for_print << "(x" << rs1 << ")";
}

       else if (std::string(mnemonic) == "jal") {
         std::cout << " x" << rd << "," << std::dec << imm_for_print;
}
       else if (std::string(mnemonic) == "jalr") {
         std::cout << " x" << rd << "," << std::dec << imm_for_print << "(x" << rs1 << ")";
}

        else if (std::string(mnemonic) == "beq" || std::string(mnemonic) == "bne" ||
         std::string(mnemonic) == "blt" || std::string(mnemonic) == "bge" ||
         std::string(mnemonic) == "bltu"|| std::string(mnemonic) == "bgeu") {
         std::cout << " x" << rs1 << ",x" << rs2 << "," << std::dec << imm_for_print;
}
       else if (std::string(mnemonic) == "lui" || std::string(mnemonic) == "auipc") {
         std::cout << " x" << rd << ",0x" << std::hex << (uint32_t)imm_for_print << std::dec;
}

       else if (std::string(mnemonic) == "fence" || std::string(mnemonic) == "fence.i") {
       // no operands to print
}
       else if (std::string(mnemonic) == "csrrw"  ||
         std::string(mnemonic) == "csrrs"  ||
         std::string(mnemonic) == "csrrc") {
         uint32_t csr_addr = get_bits(inst_before, 31, 20);
         std::cout << " x" << rd << ",0x" << std::hex << csr_addr
              << ",x" << std::dec << rs1;
}
    else if (std::string(mnemonic) == "csrrwi" ||
         std::string(mnemonic) == "csrrsi" ||
         std::string(mnemonic) == "csrrci") {
         uint32_t csr_addr = get_bits(inst_before, 31, 20);
         std::cout << " x" << rd << ",0x" << std::hex << csr_addr
              << "," << std::dec << rs1; // rs1 is zimm
}

        if (wb_reg >= 0) {
        std::cout << " WB: x" << wb_reg << "=0x"
        << std::hex << std::setw(8) << std::setfill('0') << wb_val
        << std::dec;
        }   
        std::cout << "\n";
    }
}

uint32_t CPU::csr_read(uint32_t addr) const {
return csr_[addr & 0xFFFu];
}

}