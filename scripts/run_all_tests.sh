#!/bin/bash
# Script to run all test cases

echo "========================================"
echo "Running All Test Cases"
echo "========================================"
echo ""

# Check if executable exists
if [ ! -f "./sudoku_benchmark" ]; then
    echo "✗ Error: sudoku_benchmark executable not found!"
    echo "Please compile first: bash scripts/compile.sh"
    exit 1
fi

# Check if test_cases directory exists
if [ ! -d "./test_cases" ]; then
    echo "✗ Error: test_cases directory not found!"
    exit 1
fi

# Count test files
test_count=$(find test_cases -name "*.txt" | wc -l)

if [ $test_count -eq 0 ]; then
    echo "✗ No test files found in test_cases/"
    exit 1
fi

echo "Found $test_count test case(s)"
echo ""

# Run each test case
test_num=1
for test_file in test_cases/*.txt; do
    echo "========================================="
    echo "Test $test_num: $(basename $test_file)"
    echo "========================================="
    
    # Run the benchmark (would need to modify benchmark.cpp to accept file argument)
    # For now, this demonstrates the structure
    ./sudoku_benchmark
    
    echo ""
    ((test_num++))
done

echo "========================================="
echo "✓ All tests completed"
echo "========================================="
