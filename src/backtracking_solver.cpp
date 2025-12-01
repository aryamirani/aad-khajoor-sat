/**
 * @file backtracking_solver.cpp
 * @brief Implementation of backtracking Sudoku solver with heuristics
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#include "backtracking_solver.h"
#include <vector>

BacktrackingSolver::BacktrackingSolver(const Grid& puzzle) : grid(puzzle) {}

bool BacktrackingSolver::solve(bool use_mrv, bool use_fc) {
    stats = {0, 0};
    // Domains: domains[r][c][k] is true if k (1-9) is possible for (r,c)
    Domains domains;
    initialize_domains(domains);

    return solve_recursive(use_mrv, use_fc, domains);
}

void BacktrackingSolver::initialize_domains(Domains& domains) {
    // Initialize all domains to true (all values possible)
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            for (int k = 1; k <= 9; ++k) {
                domains[r][c][k] = true;
            }
        }
    }

    // Propagate constraints from pre-filled cells
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (grid[r][c] != 0) {
                int val = grid[r][c];
                // Only this value is possible for pre-filled cell
                for (int k = 1; k <= 9; ++k) {
                    if (k != val) domains[r][c][k] = false;
                }
                // Propagate to constrained cells
                std::vector<std::pair<int, int>> dummy_changes;
                propagate_constraints(r, c, val, domains, dummy_changes);
            }
        }
    }
}

bool BacktrackingSolver::find_empty_cell(int& r, int& c) {
    // Scan grid from top-left to bottom-right
    for (r = 0; r < 9; ++r) {
        for (c = 0; c < 9; ++c) {
            if (grid[r][c] == 0) {
                return true; // Found empty cell at (r, c)
            }
        }
    }
    return false; // No empty cells - puzzle complete
}

bool BacktrackingSolver::find_mrv_cell(int& r_best, int& c_best, 
                                       const Domains& domains, bool use_fc) {
    int min_remaining = 10;
    bool found = false;
    
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (grid[r][c] == 0) {
                int remaining = 0;
                
                if (use_fc) {
                    // Domain object is reliable - use it directly
                    for (int k = 1; k <= 9; ++k) {
                        if (domains[r][c][k]) remaining++;
                    }
                } else {
                    // Domain object is stale - must recalculate
                    for (int k = 1; k <= 9; ++k) {
                        if (is_valid(r, c, k)) {
                            remaining++;
                        }
                    }
                }

                // Found a cell with no options - instant backtrack trigger
                if (remaining == 0) {
                    r_best = r;
                    c_best = c;
                    return true; // Found a "best" cell (one that will fail)
                }

                // Track cell with minimum remaining values
                if (remaining < min_remaining) {
                    min_remaining = remaining;
                    r_best = r;
                    c_best = c;
                    found = true;
                }
            }
        }
    }
    return found;
}

bool BacktrackingSolver::propagate_constraints(int r, int c, int val, 
                                               Domains& domains,
                                               std::vector<std::pair<int, int>>& changes) {
    // Remove val from all cells in same row
    for (int col = 0; col < 9; ++col) {
        if (col != c && domains[r][col][val]) {
            domains[r][col][val] = false;
            changes.push_back({r, col});
            // Check if this cell now has no valid options
            if (count_domain(r, col, domains) == 0) {
                return false; // Conflict detected - dead end
            }
        }
    }
    
    // Remove val from all cells in same column
    for (int row = 0; row < 9; ++row) {
        if (row != r && domains[row][c][val]) {
            domains[row][c][val] = false;
            changes.push_back({row, c});
            // Check if this cell now has no valid options
            if (count_domain(row, c, domains) == 0) {
                return false; // Conflict detected - dead end
            }
        }
    }
    
    // Remove val from all cells in same 3x3 block
    int start_row = 3 * (r / 3);
    int start_col = 3 * (c / 3);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            int row = start_row + i;
            int col = start_col + j;
            if (row != r && col != c && domains[row][col][val]) {
                domains[row][col][val] = false;
                changes.push_back({row, col});
                // Check if this cell now has no valid options
                if (count_domain(row, col, domains) == 0) {
                    return false; // Conflict detected - dead end
                }
            }
        }
    }
    
    return true; // No conflicts detected
}

void BacktrackingSolver::restore_constraints(int val, Domains& domains,
                                             const std::vector<std::pair<int, int>>& changes) {
    // Restore val to all modified cells' domains
    for (const auto& pos : changes) {
        domains[pos.first][pos.second][val] = true;
    }
}

int BacktrackingSolver::count_domain(int r, int c, const Domains& domains) {
    int count = 0;
    for (int k = 1; k <= 9; ++k) {
        if (domains[r][c][k]) count++;
    }
    return count;
}

bool BacktrackingSolver::is_valid(int r, int c, int k) {
    // Check row for conflict
    for (int i = 0; i < 9; ++i) {
        if (grid[r][i] == k) return false;
    }
    
    // Check column for conflict
    for (int i = 0; i < 9; ++i) {
        if (grid[i][c] == k) return false;
    }
    
    // Check 3x3 block for conflict
    int start_row = 3 * (r / 3);
    int start_col = 3 * (c / 3);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (grid[start_row + i][start_col + j] == k) {
                return false;
            }
        }
    }
    
    return true; // No conflicts
}

bool BacktrackingSolver::solve_recursive(bool use_mrv, bool use_fc, Domains domains) {
    int r, c;
    
    // Select next empty cell using chosen heuristic
    if (use_mrv) {
        if (!find_mrv_cell(r, c, domains, use_fc)) {
            // No empty cells found - puzzle solved!
            solution = grid;
            return true;
        }
    } else {
        if (!find_empty_cell(r, c)) {
            // No empty cells found - puzzle solved!
            solution = grid;
            return true;
        }
    }
    
    // Get possible values for selected cell
    std::vector<int> possible_values;
    if (use_fc) {
        // Use reliable domain information
        for (int k = 1; k <= 9; ++k) {
            if (domains[r][c][k]) {
                possible_values.push_back(k);
            }
        }
    } else {
        // Recalculate valid values
        for (int k = 1; k <= 9; ++k) {
            if (is_valid(r, c, k)) {
                possible_values.push_back(k);
            }
        }
    }

    // If no possible values, backtrack
    if (possible_values.empty() && grid[r][c] == 0) {
        stats.backtracks++;
        return false; 
    }

    // Try each possible value
    for (int k : possible_values) {
        stats.decisions++;
        grid[r][c] = k; // Make assignment

        if (use_fc) {
            // Forward checking: propagate constraints
            Domains domains_copy = domains;
            std::vector<std::pair<int, int>> changes;
            
            if (propagate_constraints(r, c, k, domains_copy, changes)) {
                // No conflicts - continue recursively
                if (solve_recursive(use_mrv, use_fc, domains_copy)) {
                    return true; // Solution found!
                }
            }
            // Conflicts detected or recursive call failed
        } else {
            // No forward checking - just recurse
            if (solve_recursive(use_mrv, use_fc, domains)) {
                return true; // Solution found!
            }
        }

        // Backtrack: undo assignment
        stats.backtracks++;
        grid[r][c] = 0;
    }
    
    return false; // All values tried - no solution on this path
}
