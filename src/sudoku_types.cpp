/**
 * @file sudoku_types.cpp
 * @brief Implementation of Sudoku utility functions
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#include "sudoku_types.h"
#include <iostream>
#include <set>

void print_grid(const Grid& grid) {
    std::cout << "-------------------------" << std::endl;
    for (int r = 0; r < 9; ++r) {
        std::cout << "| ";
        for (int c = 0; c < 9; ++c) {
            std::cout << grid[r][c] << " ";
            if ((c + 1) % 3 == 0) std::cout << "| ";
        }
        std::cout << std::endl;
        if ((r + 1) % 3 == 0) std::cout << "-------------------------" << std::endl;
    }
}

Grid load_puzzle_from_string(const std::string& s) {
    Grid grid{};
    int r = 0, c = 0;
    for (char ch : s) {
        if (ch >= '0' && ch <= '9') {
            if (c < 9) {
                grid[r][c++] = ch - '0';
            }
            if (c == 9) {
                c = 0;
                r++;
            }
            if (r == 9) break;
        }
    }
    return grid;
}

bool validate_solution(const Grid& grid) {
    // Check for any empty cells (zeros)
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (grid[r][c] == 0 || grid[r][c] < 1 || grid[r][c] > 9) {
                return false; // Invalid cell value
            }
        }
    }
    
    // Check all rows
    for (int r = 0; r < 9; ++r) {
        std::set<int> seen;
        for (int c = 0; c < 9; ++c) {
            if (seen.count(grid[r][c])) {
                return false; // Duplicate in row
            }
            seen.insert(grid[r][c]);
        }
    }
    
    // Check all columns
    for (int c = 0; c < 9; ++c) {
        std::set<int> seen;
        for (int r = 0; r < 9; ++r) {
            if (seen.count(grid[r][c])) {
                return false; // Duplicate in column
            }
            seen.insert(grid[r][c]);
        }
    }
    
    // Check all 3x3 blocks
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            std::set<int> seen;
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    int val = grid[br * 3 + r][bc * 3 + c];
                    if (seen.count(val)) {
                        return false; // Duplicate in block
                    }
                    seen.insert(val);
                }
            }
        }
    }
    
    return true; // All checks passed
}
