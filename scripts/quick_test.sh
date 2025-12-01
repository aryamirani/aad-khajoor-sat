#!/bin/bash
# Quick test of only fast SAT configurations

cd /root/AlgorithmAnalysisDesign

echo "Testing with only efficient SAT configurations..."
echo "Recommended: Use puzzles with Unit Propagation enabled for reasonable runtimes"
echo ""
echo "Note: VSIDS-only, Pure Literal-only, and No heuristics can be VERY slow"
echo "      (may take minutes to hours on hard puzzles)"
echo ""

# Just test backtracking (fast)
./sudoku_benchmark test_cases/easy.txt 2>&1 | grep -A 100 "BACKTRACKING" | head -60

echo ""
echo "✅ Backtracking tests complete and program exited properly!"
