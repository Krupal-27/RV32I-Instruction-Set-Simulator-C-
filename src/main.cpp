#include "rv/memory.hpp"
#include "rv/cpu.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    bool trace = false;
    std::string bin_path;

    // parse args
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--trace") trace = true;
        else bin_path = a;
    }

    if (bin_path.empty()) {
        std::cerr << "Usage: rv32i_iss [--trace] <test.bin>\n";
        return 1;
    }

    rv::Memory mem(64 * 1024);
    mem.load_binary(bin_path, 0);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(trace);

    try {
        while (true) cpu.step();
    } catch (...) {
        // stop
    }

    std::cout << "x3 = " << cpu.reg(3) << "\n";
    return 0;
}