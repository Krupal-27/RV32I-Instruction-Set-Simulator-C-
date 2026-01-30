#include "rv/memory.hpp"
#include "rv/cpu.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

static void test_addi_add() {
    rv::Memory mem(1024);

    // Program:
    // addi x1,x0,10
    // addi x2,x0,20
    // add  x3,x1,x2
    // ebreak
    uint32_t prog[] = {
        0x00A00093u,
        0x01400113u,
        0x002081B3u,
        0x00100073u
    };

    for (int i = 0; i < 4; i++) mem.store32(i * 4, prog[i]);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(false);

    try { while (true) cpu.step(); }
    catch (...) {}

    assert(cpu.reg(1) == 10);
    assert(cpu.reg(2) == 20);
    assert(cpu.reg(3) == 30);
}

static void test_lw_sw() {
    rv::Memory mem(1024);

    // addi x1,x0,100
    // addi x2,x0,42
    // sw   x2,0(x1)
    // lw   x3,0(x1)
    // ebreak
    uint32_t prog[] = {
        0x06400093u,
        0x02A00113u,
        0x0020A023u,
        0x0000A183u,
        0x00100073u
    };

    for (int i = 0; i < 5; i++) mem.store32(i * 4, prog[i]);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(false);

    try { while (true) cpu.step(); }
    catch (...) {}

    assert(cpu.reg(3) == 42);
    // also confirm memory really got written
    assert(mem.load32(100) == 42);
}

static void test_branch_bne_loop() {
    rv::Memory mem(1024);

    // x1=0
    // x2=5
    // loop: x1=x1+1
    // bne x1,x2, loop
    // ebreak
    uint32_t prog[] = {
        0x00000093u, // addi x1,x0,0
        0x00500113u, // addi x2,x0,5
        0x00108093u, // addi x1,x1,1
        0xFE209EE3u, // bne  x1,x2,-4
        0x00100073u  // ebreak
    };

    for (int i = 0; i < 5; i++) mem.store32(i * 4, prog[i]);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(false);

    try { while (true) cpu.step(); }
    catch (...) {}

    assert(cpu.reg(1) == 5);
}

static void test_lui_auipc() {
    rv::Memory mem(1024);

    // lui   x1,0x12345   -> x1=0x12345000
    // auipc x2,0x1       -> x2=PC(4)+0x1000 = 0x1004
    // ebreak
    uint32_t prog[] = {
        0x123450B7u,
        0x00001117u,
        0x00100073u
    };

    for (int i = 0; i < 3; i++) mem.store32(i * 4, prog[i]);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(false);

    try { while (true) cpu.step(); }
    catch (...) {}

    assert(cpu.reg(1) == 0x12345000u);
    assert(cpu.reg(2) == 0x00001004u);
}

static void test_lb_lbu_sb() {
    rv::Memory mem(1024);

    // addi x1,x0,100
    // addi x2,x0,0xFF
    // sb x2,0(x1)
    // lb x3,0(x1)
    // lbu x4,0(x1)
    // ebreak
    uint32_t prog[] = {
        0x06400093u,
        0x0FF00113u,
        0x00208023u,
        0x00008183u,
        0x0000C203u,
        0x00100073u
    };

    for (int i = 0; i < 6; i++) mem.store32(i * 4, prog[i]);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(false);

    try { while (true) cpu.step(); } catch (...) {}

    assert(cpu.reg(3) == (uint32_t)(int8_t)0xFF); // signed
    assert(cpu.reg(4) == 0xFF);                  // unsigned
}

static void test_fence_ecall() {
    rv::Memory mem(1024);

    // fence
    // ecall
    uint32_t prog[] = {
        0x0000000Fu, // fence
        0x00000073u  // ecall
    };

    for (int i = 0; i < 2; i++) mem.store32(i * 4, prog[i]);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(false);

    bool stopped = false;
    try { while (true) cpu.step(); }
    catch (...) { stopped = true; }

    assert(stopped); // should stop on ecall
}

static void test_csr_basic() {
    rv::Memory mem(1024);

    // addi  x1,x0,0x55
    // csrrw x2,mtvec,x1     (x2 gets old mtvec=0, mtvec becomes 0x55)
    // csrrs x3,mtvec,x0     (x3 reads mtvec, no write)
    // ebreak
    uint32_t prog[] = {
        0x05500093u, // addi  x1,x0,85
        0x30509173u, // csrrw x2,0x305,x1
        0x305021F3u, // csrrs x3,0x305,x0
        0x00100073u  // ebreak
    };

    for (int i = 0; i < 4; i++) mem.store32(i * 4, prog[i]);

    rv::CPU cpu(mem);
    cpu.reset(0);
    cpu.set_trace(false);

    try { while (true) cpu.step(); } catch (...) {}

    assert(cpu.reg(2) == 0u);
    assert(cpu.reg(3) == 0x55u);
    assert(cpu.csr_read(0x305) == 0x55u);
}





int main() {
    test_addi_add();
    test_lw_sw();
    test_branch_bne_loop();
    test_lui_auipc();
    test_lb_lbu_sb();
    test_fence_ecall();
    test_csr_basic();



    std::cout << "All tests passed!\n";
    return 0;
}

