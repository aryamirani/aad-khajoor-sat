#!/bin/bash
# Compilation script for Sudoku SAT Solver

echo "========================================"
echo "Compiling SAT Solver & Sudoku Benchmark"
echo "========================================"
echo ""

# Check if g++ is available
if ! command -v g++ &> /dev/null; then
    echo "✗ Error: g++ compiler not found!"
    echo "Please install g++ (version 11 or later)"
    exit 1
fi

# Display compiler version
echo "Using compiler:"
g++ --version | head -n 1
echo ""

# Compile command
echo "Compiling with optimization level -O3..."
g++ -std=c++17 -O3 -Wall -Wextra \
    -o sudoku_benchmark \
    src/*.cpp benchmark.cpp \
    -I./include \
    -lpthread

# Check compilation result
if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "✓ Compilation successful!"
    echo "========================================"
    echo ""
    echo "Run with: ./sudoku_benchmark"
    echo ""
else
    echo ""
    echo "========================================"
    echo "✗ Compilation failed!"
    echo "========================================"
    echo "Please check the error messages above"
    exit 1
fi
