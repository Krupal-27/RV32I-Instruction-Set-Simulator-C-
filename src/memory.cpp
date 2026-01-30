#include "rv/memory.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <sstream>

namespace rv {

Memory::Memory(std::size_t size_bytes)
    : mem_(size_bytes, 0) {}



void Memory::check_addr(std::uint32_t addr, std::size_t nbytes) const {
    if (static_cast<std::size_t>(addr) + nbytes > mem_.size()) {
        std::ostringstream oss;
        oss << "Memory access out of range: addr=0x"
            << std::hex << addr << " nbytes=" << std::dec << nbytes
            << " mem_size=" << mem_.size();
        throw std::out_of_range(oss.str());
    }
}

void Memory::load_binary(const std::string& path, std::uint32_t base) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open binary file: " + path);
    }

    std::vector<std::uint8_t> buf(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    check_addr(base, buf.size());

    for (std::size_t i = 0; i < buf.size(); ++i) {
        mem_[static_cast<std::size_t>(base) + i] = buf[i];
    }
}

std::uint8_t Memory::load8(std::uint32_t addr) const {
    check_addr(addr, 1);
    return mem_[static_cast<std::size_t>(addr)];
}

void Memory::store8(std::uint32_t addr, std::uint8_t value) {
    check_addr(addr, 1);
    mem_[static_cast<std::size_t>(addr)] = value;
}

uint16_t Memory::load16(uint32_t addr) const {
    if ((addr & 0x1u) != 0) {
        throw std::runtime_error("Misaligned load16 at addr=" + std::to_string(addr));
    }
    check_addr(addr, 2);
    size_t a = static_cast<size_t>(addr);
    uint16_t b0 = mem_[a + 0];
    uint16_t b1 = mem_[a + 1];
    return (uint16_t)(b0 | (b1 << 8));
}

void Memory::store16(uint32_t addr, uint16_t value) {
    if ((addr & 0x1u) != 0) {
        throw std::runtime_error("Misaligned store16 at addr=" + std::to_string(addr));
    }
    check_addr(addr, 2);
    size_t a = static_cast<size_t>(addr);
    mem_[a + 0] = (uint8_t)(value & 0xFF);
    mem_[a + 1] = (uint8_t)((value >> 8) & 0xFF);
}

std::uint32_t Memory::load32(std::uint32_t addr) const {
    if ((addr & 0x3u) != 0) {
        throw std::runtime_error("Misaligned load32 at addr=0x" + std::to_string(addr));
    }

    check_addr(addr, 4);

    const std::size_t a = static_cast<std::size_t>(addr);
    std::uint32_t b0 = mem_[a + 0];
    std::uint32_t b1 = mem_[a + 1];
    std::uint32_t b2 = mem_[a + 2];
    std::uint32_t b3 = mem_[a + 3];

    return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void Memory::store32(std::uint32_t addr, std::uint32_t value) {
    if ((addr & 0x3u) != 0) {
        throw std::runtime_error("Misaligned store32 at addr=0x" + std::to_string(addr));
    }

    check_addr(addr, 4);

    const std::size_t a = static_cast<std::size_t>(addr);
    mem_[a + 0] = static_cast<std::uint8_t>(value & 0xFF);
    mem_[a + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    mem_[a + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    mem_[a + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
}

} // namespace rv