#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace rv {

class Memory {
public:
    explicit Memory(std::size_t size_bytes);

    void load_binary(const std::string& path, uint32_t base = 0);

    // Byte access
    uint8_t load8(uint32_t addr) const;
    void store8(uint32_t addr, uint8_t value);

    uint16_t load16(uint32_t addr) const;
    void store16(uint32_t addr, uint16_t value);

    // 32-bit access (little-endian)
    uint32_t load32(uint32_t addr) const;
    void store32(uint32_t addr, uint32_t value);

    std::size_t size() const { return mem_.size(); }

private:
    std::vector<uint8_t> mem_;
    void check_addr(uint32_t addr, std::size_t nbytes) const;
};

} // namespace rv