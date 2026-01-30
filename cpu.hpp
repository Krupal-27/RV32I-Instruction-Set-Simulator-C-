
#pragma once
#include <array>
#include <cstdint>


namespace rv {

class Memory;

class CPU {
public:
    explicit CPU(Memory& mem);

    void reset(uint32_t pc_start = 0);
    void step();

    uint32_t reg(int i) const { return regs_[i]; }
    void set_trace(bool on) { trace_ = on; }
    bool trace_enabled() const { return trace_; }
    
    uint32_t csr_read(uint32_t addr) const;
    void csr_write(uint32_t addr, uint32_t value);
    
    

private:
    Memory& mem_;
    uint32_t pc_ = 0;
    std::array<uint32_t, 32> regs_{};
    bool trace_ = false;
    std::array<uint32_t, 4096> csr_{}; // 4096 CSRs (12-bit address space)

};

} // namespace rv
