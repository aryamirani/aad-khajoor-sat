/**
 * @file sat_solver.cpp
 * @brief Implementation of DPLL-based SAT solver for Sudoku
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#include "sat_solver.h"
#include <cmath>
#include <algorithm>

SudokuSATSolver::SudokuSATSolver(const Grid& grid) {
    generate_clauses(grid);
    vsids_scores.resize(num_vars + 1, 0.0);
}

int SudokuSATSolver::to_var(int r, int c, int k) {
    // Convert (row, col, digit) to variable index
    // Formula: row * 81 + col * 9 + digit
    return r * 81 + c * 9 + k;
}

void SudokuSATSolver::generate_clauses(const Grid& grid) {
    clauses.clear();
    
    // Helper lambda to add clauses from sets
    auto add_clause = [this](const std::set<int>& lits) {
        clauses.push_back(std::vector<int>(lits.begin(), lits.end()));
    };

    // R1: Each cell has at least one digit (81 clauses)
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            std::set<int> clause;
            for (int k = 1; k <= 9; ++k) {
                clause.insert(to_var(r, c, k));
            }
            add_clause(clause);
        }
    }

    // R2: Each cell has at most one digit (2,916 clauses)
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            for (int k1 = 1; k1 <= 9; ++k1) {
                for (int k2 = k1 + 1; k2 <= 9; ++k2) {
                    // ¬V(r,c,k1) ∨ ¬V(r,c,k2)
                    add_clause({-to_var(r, c, k1), -to_var(r, c, k2)});
                }
            }
        }
    }

    // R3: Each row contains each digit exactly once (810 clauses)
    for (int r = 0; r < 9; ++r) {
        for (int k = 1; k <= 9; ++k) {
            // At least one cell in row r has digit k
            std::set<int> clause;
            for (int c = 0; c < 9; ++c) {
                clause.insert(to_var(r, c, k));
            }
            add_clause(clause);
            
            // At most one cell in row r has digit k
            for (int c1 = 0; c1 < 9; ++c1) {
                for (int c2 = c1 + 1; c2 < 9; ++c2) {
                    add_clause({-to_var(r, c1, k), -to_var(r, c2, k)});
                }
            }
        }
    }
    
    // R4: Each column contains each digit exactly once (810 clauses)
    for (int c = 0; c < 9; ++c) {
        for (int k = 1; k <= 9; ++k) {
            // At least one cell in column c has digit k
            std::set<int> clause;
            for (int r = 0; r < 9; ++r) {
                clause.insert(to_var(r, c, k));
            }
            add_clause(clause);
            
            // At most one cell in column c has digit k
            for (int r1 = 0; r1 < 9; ++r1) {
                for (int r2 = r1 + 1; r2 < 9; ++r2) {
                    add_clause({-to_var(r1, c, k), -to_var(r2, c, k)});
                }
            }
        }
    }
    
    // R5: Each 3×3 block contains each digit exactly once (810 clauses)
    for (int br = 0; br < 3; ++br) {
        for (int bc = 0; bc < 3; ++bc) {
            for (int k = 1; k <= 9; ++k) {
                // At least one cell in block (br, bc) has digit k
                std::set<int> clause;
                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) {
                        clause.insert(to_var(br * 3 + r, bc * 3 + c, k));
                    }
                }
                add_clause(clause);

                // At most one cell in block (br, bc) has digit k
                for (int r1 = 0; r1 < 3; ++r1) {
                    for (int c1 = 0; c1 < 3; ++c1) {
                        for (int r2 = 0; r2 < 3; ++r2) {
                            for (int c2 = 0; c2 < 3; ++c2) {
                                if (r1 * 3 + c1 >= r2 * 3 + c2) continue;
                                add_clause({-to_var(br*3+r1, bc*3+c1, k), 
                                           -to_var(br*3+r2, bc*3+c2, k)});
                            }
                        }
                    }
                }
            }
        }
    }

    // R6: Pre-filled cells (varies by puzzle)
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (grid[r][c] != 0) {
                // Unit clause: V(r,c,grid[r][c]) must be true
                add_clause({to_var(r, c, grid[r][c])});
            }
        }
    }
}

bool SudokuSATSolver::solve(Heuristics h) {
    config = h;
    stats = {0, 0, 0};
    model.assign(num_vars + 1, 0);
    vsids_scores.assign(num_vars + 1, 0.0);

    // Initialize VSIDS scores from clause occurrences
    if (config.vsids) {
        for (const auto& clause : clauses) {
            for (int lit : clause) {
                vsids_scores[std::abs(lit)] += 1.0;
            }
        }
    }
    
    std::vector<int> trail;
    return dpll(trail);
}

int SudokuSATSolver::bcp(std::vector<int>& trail, bool do_propagation) {
    if (!do_propagation) {
        // When propagation is disabled, just check for immediate conflicts
        // without propagating unit clauses
        for (const auto& clause : clauses) {
            int false_count = 0;
            bool satisfied = false;

            for (int lit : clause) {
                int var = std::abs(lit);
                int val = (lit > 0) ? 1 : -1;
                
                if (model[var] == val) {
                    satisfied = true;
                    break;
                }
                if (model[var] == -val) {
                    false_count++;
                }
            }

            // Check if clause becomes empty (all literals false)
            if (!satisfied && false_count == static_cast<int>(clause.size())) {
                return 0; // Conflict detected - no valid assignment possible
            }
        }
        return 1; // No conflicts
    }
    
    // Normal BCP with unit propagation
    int head = 0;
    while (head < static_cast<int>(trail.size())) {
        int assigned_lit = trail[head++];
        
        for (const auto& clause : clauses) {
            int unassigned_lit = 0;
            int false_count = 0;
            bool satisfied = false;

            for (int lit : clause) {
                int var = std::abs(lit);
                int val = (lit > 0) ? 1 : -1;
                
                if (model[var] == val) {
                    satisfied = true;
                    break;
                }
                if (model[var] == -val) {
                    false_count++;
                }
                if (model[var] == 0) {
                    unassigned_lit = lit;
                }
            }

            if (satisfied) continue;
            
            // Conflict: all literals are false
            if (false_count == static_cast<int>(clause.size())) {
                return 0; // Conflict detected
            }
            
            // Unit clause: only one literal unassigned
            if (false_count == static_cast<int>(clause.size()) - 1 && unassigned_lit != 0) {
                int var = std::abs(unassigned_lit);
                if (model[var] == 0) {
                    stats.unit_props++;
                    model[var] = (unassigned_lit > 0) ? 1 : -1;
                    trail.push_back(unassigned_lit);
                }
            }
        }
    }
    return 1; // No conflicts
}

void SudokuSATSolver::pure_literal_assign(std::vector<int>& trail) {
    std::map<int, int> lit_counts; // 1=pos, -1=neg, 0=both
    
    // Scan all unsatisfied clauses
    for (const auto& clause : clauses) {
        bool satisfied = false;
        
        // Check if clause is already satisfied
        for (int lit : clause) {
            if (model[std::abs(lit)] == ((lit > 0) ? 1 : -1)) {
                satisfied = true;
                break;
            }
        }
        if (satisfied) continue;
        
        // Track polarity of unassigned literals
        for (int lit : clause) {
            int var = std::abs(lit);
            if (model[var] != 0) continue;
            
            int val = (lit > 0) ? 1 : -1;
            if (lit_counts.find(var) == lit_counts.end()) {
                lit_counts[var] = val;
            } else if (lit_counts[var] != val) {
                lit_counts[var] = 0; // Both polarities seen - not pure
            }
        }
    }

    // Assign all pure literals
    for (auto const& [var, val] : lit_counts) {
        if (val != 0 && model[var] == 0) {
            model[var] = val;
            trail.push_back(val * var);
        }
    }
}

int SudokuSATSolver::pick_branch_var() {
    if (config.vsids) {
        // Use VSIDS heuristic: select variable with highest activity
        double max_score = -1.0;
        int best_var = 0;
        for (int var = 1; var <= num_vars; ++var) {
            if (model[var] == 0 && vsids_scores[var] > max_score) {
                max_score = vsids_scores[var];
                best_var = var;
            }
        }
        if (best_var != 0) return best_var;
    }

    // Fallback: simple first-unassigned
    for (int var = 1; var <= num_vars; ++var) {
        if (model[var] == 0) return var;
    }
    return 0; // All variables assigned
}

void SudokuSATSolver::decay_vsids_scores() {
    // Decay all scores to prioritize recent conflicts
    for (int var = 1; var <= num_vars; ++var) {
        vsids_scores[var] *= 0.95;
    }
}

bool SudokuSATSolver::dpll(std::vector<int>& trail) {
    int trail_head = trail.size();
    
    // Boolean Constraint Propagation
    if (bcp(trail, config.unit_prop) == 0) {
        // Conflict detected - backtrack
        stats.backtracks++;
        for (int i = trail_head; i < static_cast<int>(trail.size()); ++i) {
            model[std::abs(trail[i])] = 0;
        }
        trail.resize(trail_head);
        return false;
    }
    
    // Pure Literal Elimination
    if (config.pure_literal) {
        pure_literal_assign(trail);
    }

    // Select next variable to branch on
    int var = pick_branch_var();
    if (var == 0) {
        return true; // All variables assigned - solution found!
    }

    stats.decisions++;
    
    // Try assigning variable to true
    model[var] = 1;
    trail.push_back(var);
    if (dpll(trail)) return true;
    
    // Backtrack: undo all assignments made in this branch
    for (int i = trail_head; i < static_cast<int>(trail.size()); ++i) {
        model[std::abs(trail[i])] = 0;
    }
    trail.resize(trail_head);

    // Try assigning variable to false
    model[var] = -1;
    trail.push_back(-var);
    if (dpll(trail)) return true;

    // Backtrack: both polarities failed
    for (int i = trail_head; i < static_cast<int>(trail.size()); ++i) {
        model[std::abs(trail[i])] = 0;
    }
    trail.resize(trail_head);
    
    // Update VSIDS scores on conflict
    if (config.vsids) {
        vsids_scores[var] += 1.0; // Bump score on conflict
        if (stats.decisions % 100 == 0) {
            decay_vsids_scores(); // Periodic decay
        }
    }

    return false; // No solution on this path
}

Grid SudokuSATSolver::get_solution_grid() {
    Grid solution_grid{};
    
    // Decode SAT solution back to Sudoku grid
    for (int var = 1; var <= num_vars; ++var) {
        if (model[var] == 1) {
            // Variable is true - decode to (row, col, digit)
            int r = (var - 1) / 81;
            int c = ((var - 1) % 81) / 9;
            int k = ((var - 1) % 9) + 1;
            solution_grid[r][c] = k;
        }
    }
    
    return solution_grid;
}
