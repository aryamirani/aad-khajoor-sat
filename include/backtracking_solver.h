/**
 * @file backtracking_solver.h
 * @brief Backtracking-based Sudoku solver with heuristic optimizations
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#ifndef BACKTRACKING_SOLVER_H
#define BACKTRACKING_SOLVER_H

#include <array>
#include <vector>
#include "sudoku_types.h"

/**
 * @class BacktrackingSolver
 * @brief Solves Sudoku puzzles using recursive backtracking with optional heuristics
 * 
 * This class implements a Constraint Satisfaction Problem (CSP) solver specifically
 * designed for 9×9 Sudoku puzzles. It supports two key optimizations:
 * 
 * **Heuristics:**
 * - **MRV (Minimum Remaining Values):** Selects the most constrained variable first,
 *   reducing the branching factor and failing faster on impossible paths
 * - **Forward Checking:** Maintains domain consistency by propagating constraints
 *   after each assignment, enabling early detection of dead ends
 * 
 * The solver operates by recursively assigning values to empty cells,
 * propagating constraints, and backtracking when conflicts are detected.
 * 
 * **Algorithm Overview:**
 * 1. Select an empty cell (using MRV if enabled)
 * 2. Try each valid value for that cell
 * 3. Propagate constraints (if Forward Checking enabled)
 * 4. Recursively solve the remaining puzzle
 * 5. Backtrack if no valid assignment exists
 * 
 * @note All algorithms are implemented from scratch without external libraries
 * @note Statistics are collected for performance analysis
 */
class BacktrackingSolver {
public:
    /// The current state of the Sudoku grid during solving
    Grid grid;
    
    /// The final solution if one is found
    Grid solution;
    
    /**
     * @struct Statistics
     * @brief Performance metrics collected during solving
     */
    struct Statistics {
        long long decisions = 0;   ///< Number of value assignments attempted
        long long backtracks = 0;  ///< Number of times backtracking occurred
    } stats;
    
    /**
     * @brief Constructs a solver for the given puzzle
     * 
     * @param puzzle Initial Sudoku grid with 0 representing empty cells
     * 
     * The puzzle is copied internally, so modifications to the original
     * grid after construction do not affect the solver.
     */
    explicit BacktrackingSolver(const Grid& puzzle);
    
    /**
     * @brief Solves the puzzle using specified heuristics
     * 
     * @param use_mrv If true, uses Minimum Remaining Values heuristic for cell selection
     * @param use_fc If true, uses Forward Checking for constraint propagation
     * 
     * @return bool True if puzzle is solvable, false otherwise
     * 
     * @note If solvable, the solution is stored in the `solution` member
     * @note Statistics are accumulated in the `stats` member
     * @note The original `grid` member is restored if solving fails
     * 
     * Time Complexity: O(9^(n*n)) worst case, where n is the number of empty cells
     * Space Complexity: O(n*n) for recursion stack and domain tracking
     */
    bool solve(bool use_mrv, bool use_fc);
    
private:
    /**
     * @typedef Domains
     * @brief Tracks possible values for each cell
     * 
     * domains[row][col][value] is true if value (1-9) is possible for cell (row, col)
     * Index 0 is unused; indices 1-9 correspond to digit values
     */
    using Domains = std::array<std::array<std::array<bool, 10>, 9>, 9>;
    
    /**
     * @brief Initializes domain tracking based on initial puzzle state
     * 
     * For each filled cell, removes that digit from the domains of all cells
     * in the same row, column, and 3x3 block.
     * 
     * @param domains Domain structure to populate
     * 
     * Time Complexity: O(n²) where n=9
     * Space Complexity: O(1) - modifies parameter in-place
     */
    void initialize_domains(Domains& domains);
    
    /**
     * @brief Finds the first empty cell in the grid (simple heuristic)
     * 
     * Scans the grid from left-to-right, top-to-bottom and returns the
     * first cell containing 0 (empty cell). This is the baseline approach
     * without any variable ordering heuristic.
     * 
     * @param[out] r Row index of empty cell (0-8)
     * @param[out] c Column index of empty cell (0-8)
     * 
     * @return bool True if an empty cell exists, false if grid is complete
     * 
     * @see find_mrv_cell() For the MRV heuristic alternative
     * 
     * Time Complexity: O(n²) where n=9
     * Space Complexity: O(1)
     */
    bool find_empty_cell(int& r, int& c);
    
    /**
     * @brief Finds empty cell with minimum remaining values (MRV heuristic)
     * 
     * Implements the Minimum Remaining Values (MRV) heuristic, also known as
     * "most constrained variable" or "fail-first" heuristic. Selects the empty
     * cell with the fewest legal values remaining.
     * 
     * This heuristic significantly improves performance by:
     * - Detecting failures earlier (cells with 0 options)
     * - Reducing branching factor in the search tree
     * - Focusing on the most constrained parts of the puzzle
     * 
     * @param[out] r_best Row index of best cell to try next
     * @param[out] c_best Column index of best cell to try next
     * @param domains Current domain state
     * @param use_fc Whether forward checking is enabled (affects domain reliability)
     * 
     * @return bool True if empty cell found, false if grid is complete
     * 
     * @note If use_fc is false, domains are recalculated for each cell
     * @note If a cell with 0 options is found, it's immediately returned
     * 
     * Time Complexity: O(n²·d) where n=9, d=9 (domain size)
     * Space Complexity: O(1)
     */
    bool find_mrv_cell(int& r_best, int& c_best, 
                       const Domains& domains, bool use_fc);
    
    /**
     * @brief Propagates constraints after assigning value to cell
     * 
     * Implements Forward Checking by removing the assigned value from the domains
     * of all cells in the same row, column, and 3x3 block. Tracks all changes
     * to enable rollback during backtracking.
     * 
     * @param r Row of assigned cell (0-8)
     * @param c Column of assigned cell (0-8)
     * @param val Value assigned to cell (1-9)
     * @param domains Domain structure to update
     * @param changes Vector to track which domains were modified (for rollback)
     * 
     * @return bool False if a conflict is detected (any domain becomes empty),
     *              true otherwise
     * 
     * @note Changes are recorded as (row, col) pairs in the changes vector
     * 
     * Time Complexity: O(n) where n=9 (checks 27 cells: 9 row + 9 col + 9 block)
     * Space Complexity: O(n) for change tracking
     */
    bool propagate_constraints(int r, int c, int val,
                               Domains& domains,
                               std::vector<std::pair<int, int>>& changes);
    
    /**
     * @brief Restores domains after backtracking
     * 
     * Undoes the domain modifications made by propagate_constraints() when
     * backtracking from a dead end. Re-enables the specified value in all
     * affected cells' domains.
     * 
     * @param val Value that was unassigned (1-9)
     * @param domains Domain structure to restore
     * @param changes List of (row, col) pairs that were modified
     * 
     * Time Complexity: O(m) where m is the number of changes
     * Space Complexity: O(1)
     */
    void restore_constraints(int val, Domains& domains,
                            const std::vector<std::pair<int, int>>& changes);
    
    /**
     * @brief Counts remaining legal values for a cell
     * 
     * Used by the MRV heuristic to determine which cell is most constrained.
     * 
     * @param r Row index (0-8)
     * @param c Column index (0-8)
     * @param domains Current domain state
     * 
     * @return int Number of possible values (0-9)
     * 
     * Time Complexity: O(d) where d=9 (domain size)
     * Space Complexity: O(1)
     */
    int count_domain(int r, int c, const Domains& domains);
    
    /**
     * @brief Checks if assigning value k to cell (r,c) is valid
     * 
     * Validates that the assignment doesn't violate Sudoku constraints by
     * checking the row, column, and 3x3 block for conflicts.
     * 
     * Used when Forward Checking is disabled to determine valid moves.
     * 
     * @param r Row index (0-8)
     * @param c Column index (0-8)
     * @param k Value to test (1-9)
     * 
     * @return bool True if assignment doesn't violate constraints
     * 
     * Time Complexity: O(n) where n=9
     * Space Complexity: O(1)
     */
    bool is_valid(int r, int c, int k);
    
    /**
     * @brief Recursive backtracking search implementation
     * 
     * Core solving algorithm that recursively explores the search space:
     * 1. Select next empty cell (using heuristic if enabled)
     * 2. Try each valid value for that cell
     * 3. Propagate constraints (if FC enabled)
     * 4. Recursively solve remaining puzzle
     * 5. Backtrack on failure
     * 
     * @param use_mrv Whether to use MRV heuristic for cell selection
     * @param use_fc Whether to use Forward Checking for propagation
     * @param domains Current domain state (passed by value for proper backtracking)
     * 
     * @return bool True if solution found, false if current path is impossible
     * 
     * @note Modifies the `grid` member during search
     * @note Updates `stats` counters for decisions and backtracks
     * 
     * Time Complexity: O(9^(n*n)) worst case
     * Space Complexity: O(n*n) for recursion depth and domain copies
     */
    bool solve_recursive(bool use_mrv, bool use_fc, Domains domains);
};

#endif // BACKTRACKING_SOLVER_H
