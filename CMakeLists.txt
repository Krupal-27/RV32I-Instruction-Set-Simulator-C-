cmake_minimum_required(VERSION 3.20)
project(rv32i_iss LANGUAGES CXX)

# Main ISS executable
add_executable(rv32i_iss
    src/main.cpp
    src/memory.cpp
    src/cpu.cpp
)

target_include_directories(rv32i_iss PRIVATE include)
target_compile_features(rv32i_iss PRIVATE cxx_std_20)

# ----------------------------
# Tests (Step 9)
# ----------------------------
enable_testing()

add_executable(rv32i_tests
    tests/test_rv32i.cpp
    src/memory.cpp
    src/cpu.cpp
)

target_include_directories(rv32i_tests PRIVATE include)
target_compile_features(rv32i_tests PRIVATE cxx_std_20)

add_test(NAME rv32i_tests COMMAND rv32i_tests)