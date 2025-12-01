/**
 * @file sudoku_types.h
 * @brief Core data types and utility functions for Sudoku puzzles
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#ifndef SUDOKU_TYPES_H
#define SUDOKU_TYPES_H

#include <array>
#include <string>

/**
 * @typedef Grid
 * @brief Represents a 9x9 Sudoku grid
 * 
 * Grid cells contain values 0-9, where 0 represents an empty cell
 * and 1-9 represent filled cells with those digits.
 * 
 * Grid[row][column] accesses cell at (row, column), both 0-indexed.
 */
using Grid = std::array<std::array<int, 9>, 9>;

/**
 * @brief Prints a Sudoku grid to stdout with formatting
 * 
 * Displays the grid with box separators for readability:
 * - Horizontal lines separate 3x3 blocks
 * - Vertical bars separate 3x3 blocks
 * 
 * @param grid The 9x9 Sudoku grid to display
 * 
 * Time Complexity: O(1) - always prints 81 cells
 * Space Complexity: O(1)
 */
void print_grid(const Grid& grid);

/**
 * @brief Loads a Sudoku puzzle from a string representation
 * 
 * Parses a string containing 81 digits (0-9) and constructs a Grid.
 * Non-digit characters (spaces, newlines) are ignored.
 * 
 * Format: "123456789012345678901234567890..." (81 digits total)
 * - '0' represents empty cells
 * - '1'-'9' represent filled cells
 * 
 * @param s String containing puzzle data (must have at least 81 digits)
 * @return Grid The constructed 9x9 Sudoku grid
 * 
 * @warning If string has fewer than 81 digits, remaining cells are 0
 * 
 * Time Complexity: O(n) where n is string length
 * Space Complexity: O(1) - grid is fixed size
 */
Grid load_puzzle_from_string(const std::string& s);

/**
 * @brief Validates that a completed grid satisfies all Sudoku constraints
 * 
 * Checks that the solution is correct:
 * - Each row contains digits 1-9 exactly once
 * - Each column contains digits 1-9 exactly once
 * - Each 3x3 block contains digits 1-9 exactly once
 * - No cell contains 0 (all cells filled)
 * 
 * @param grid The 9x9 Sudoku grid to validate
 * @return bool True if grid is a valid complete solution, false otherwise
 * 
 * Time Complexity: O(1) - always checks 81 cells and 27 constraints
 * Space Complexity: O(1)
 */
bool validate_solution(const Grid& grid);

#endif // SUDOKU_TYPES_H
