Perfect 👍
Below is a **complete, polished README.md** written **as if all your project files are present** (`cpu.cpp/.hpp`, `memory.cpp/.hpp`, `main.cpp`, `test_rv32i.cpp`) **and accurately reflecting your actual implementation**, including **CSR instructions and SYSTEM handling**.

This is **GitHub-ready** and **interview-safe**.

---

# RISC-V RV32I CPU Simulator with CSR Support (C++)

## 📌 Overview

This project is a **C++ implementation of a RISC-V RV32I CPU simulator**, designed to model the core behavior of a 32-bit RISC-V processor.
It focuses on **instruction-level execution**, including arithmetic, memory access, control flow, and **SYSTEM/CSR instructions**.

The simulator is intended for **educational use**, CPU architecture learning, and as a **strong portfolio project** for computer architecture, VLSI, and embedded systems roles.

---

## ✨ Key Features

### 🧠 Core CPU Functionality

* RV32I **fetch–decode–execute** cycle
* 32 general-purpose registers (`x0–x31`)
* Program Counter (PC) management
* Little-endian memory model
* Load/store instructions (`lb`, `lbu`, `lw`, `sb`, `sw`)
* Arithmetic & logical instructions (`add`, `addi`, etc.)
* Control flow (`bne`, `lui`, `auipc`)

---

### ⚙️ SYSTEM & CSR Support

* SYSTEM opcode (`0x73`) decoding
* **CSR instructions implemented**:

  * `CSRRW`, `CSRRS`, `CSRRC`
  * `CSRRWI`, `CSRRSI`, `CSRRCI`
* Correct CSR semantics:

  * `rd` receives **old CSR value**
  * Conditional write suppression per RISC-V spec
* CSR storage accessed via `csr_read()` / `csr_write()`

#### ECALL / EBREAK

* `ECALL (0x00000073)` → stops execution
* `EBREAK (0x00100073)` → stops execution
* Used as **controlled termination points**
* Full trap/interrupt redirection is intentionally **not implemented**

> ℹ️ This project implements **CSR instruction behavior**, but not the full privileged architecture (no privilege modes, interrupts, or `mret`).

---

## 📂 Project Structure

```
├── cpu.hpp         # CPU class definition
├── cpu.cpp         # Instruction decode & execution logic
├── memory.hpp      # Memory interface
├── memory.cpp      # Memory implementation
├── main.cpp        # Example program runner
├── test_rv32i.cpp  # Unit tests for RV32I & CSR instructions
└── README.md
```

---

## 🧪 Testing

The included test suite (`test_rv32i.cpp`) verifies:

* Arithmetic instructions (`add`, `addi`)
* Memory operations (`lw`, `sw`, `lb`, `lbu`, `sb`)
* Branching using `bne`
* Immediate & upper instructions (`lui`, `auipc`)
* `fence` treated as NOP
* `ecall` used as execution stop
* CSR read/write correctness (example CSR address: `0x305`)

Successful execution prints:

```
All tests passed!
```

---

## ⚙️ Build & Run

### Compile Tests

```bash
g++ -std=c++17 cpu.cpp memory.cpp test_rv32i.cpp -o rv32i_tests
```

### Run Tests

```bash
./rv32i_tests
```

---

### Compile Simulator

```bash
g++ -std=c++17 cpu.cpp memory.cpp main.cpp -o rv32i_sim
```

### Run

```bash
./rv32i_sim
```

---

## 🧠 Design Notes

* The simulator is **single-cycle, instruction-accurate**, not pipelined
* `x0` is hardwired to zero (RISC-V compliant)
* CSR immediate instructions correctly treat `rs1` as `zimm`
* `fence` is implemented as a no-op (acceptable for functional simulation)
* Errors like illegal instructions throw runtime exceptions

---

## 🚀 Possible Extensions

* Full Machine-mode CSR model (`mstatus`, `mepc`, `mcause`, `mtvec`)
* Trap & exception redirection
* Privilege levels (U/M)
* `mret` instruction
* ELF binary loader
* RV32M (mul/div) extension
* Pipeline or cycle-accurate simulation

---

## 🎯 Learning Outcomes

* Deep understanding of RISC-V RV32I ISA
* Practical experience with CSR semantics
* Instruction decoding and bitfield extraction
* CPU–memory interaction modeling in C++
* Clean, modular system design

---

## 👤 Author

**Krupal Ashokkumar Babariya**
M.Sc. Electrical and Microsystem Engineering
Background in Chemistry with strong interest in **computer architecture, RISC-V, and semiconductor systems**




