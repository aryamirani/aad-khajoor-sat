#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <array>
#include <numeric>
#include <algorithm>
#include <map>
#include <set>
#include <cmath>
#include <future> // For std::async and std::future
#include <memory> // For std::shared_ptr

// --- Timer Utility ---
class Timer {
public:
    Timer() : start_time(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    double elapsed_s() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;
        return diff.count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

// --- Grid Representation ---
using Grid = std::array<std::array<int, 9>, 9>;

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

// --- Backtracking Solver ---
class BacktrackingSolver {
public:
    Grid grid{};
    Grid solution{};
    struct {
        long long decisions = 0;
        long long backtracks = 0;
    } stats;

    BacktrackingSolver(const Grid& puzzle) : grid(puzzle) {}

    bool solve(bool use_mrv, bool use_fc) {
        stats = {0, 0};
        // Domains: domains[r][c][k] is true if k (1-9) is possible for (r,c)
        std::array<std::array<std::array<bool, 10>, 9>, 9> domains;
        initialize_domains(domains);

        return solve_recursive(use_mrv, use_fc, domains);
    }

    void initialize_domains(std::array<std::array<std::array<bool, 10>, 9>, 9>& domains) {
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                for (int k = 1; k <= 9; ++k) {
                    domains[r][c][k] = true;
                }
            }
        }

        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if (grid[r][c] != 0) {
                    int val = grid[r][c];
                    for (int k = 1; k <= 9; ++k) {
                        if (k != val) domains[r][c][k] = false;
                    }
                    std::vector<std::pair<int, int>> dummy_changes;
                    propagate_constraints(r, c, val, domains, dummy_changes); // Initial propagation
                }
            }
        }
    }

    // --- Heuristic 1: Find Empty Cell (Simple) ---
    bool find_empty_cell(int& r, int& c) {
        for (r = 0; r < 9; ++r) {
            for (c = 0; c < 9; ++c) {
                if (grid[r][c] == 0) return true;
            }
        }
        return false;
    }

    // --- Heuristic 2: Find Empty Cell (MRV) ---
    bool find_mrv_cell(int& r_best, int& c_best, const std::array<std::array<std::array<bool, 10>, 9>, 9>& domains, bool use_fc) {
        int min_remaining = 10;
        bool found = false;
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if (grid[r][c] == 0) {
                    int remaining = 0;
                    if (use_fc) { // Domain object is reliable
                        for (int k = 1; k <= 9; ++k) {
                            if (domains[r][c][k]) remaining++;
                        }
                    } else { // Domain object is stale, must recalculate
                        for (int k = 1; k <= 9; ++k) {
                            if (is_valid(r, c, k)) {
                                remaining++;
                            }
                        }
                    }

                    if (remaining == 0) { // Found a cell with no options, instant backtrack
                        r_best = r;
                        c_best = c;
                        return true; // Found a "best" cell (one that will fail)
                    }

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

    // --- Heuristic 3: Forward Checking (as propagation) ---
    bool propagate_constraints(int r, int c, int val, 
                               std::array<std::array<std::array<bool, 10>, 9>, 9>& domains,
                               std::vector<std::pair<int, int>>& changes) {
        // Row
        for (int col = 0; col < 9; ++col) {
            if (col != c && domains[r][col][val]) {
                domains[r][col][val] = false;
                changes.push_back({r, col});
                if (count_domain(r, col, domains) == 0) return false;
            }
        }
        // Column
        for (int row = 0; row < 9; ++row) {
            if (row != r && domains[row][c][val]) {
                domains[row][c][val] = false;
                changes.push_back({row, c});
                if (count_domain(row, c, domains) == 0) return false;
            }
        }
        // Box
        int start_row = 3 * (r / 3);
        int start_col = 3 * (c / 3);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                int row = start_row + i;
                int col = start_col + j;
                if (row != r && col != c && domains[row][col][val]) {
                    domains[row][col][val] = false;
                    changes.push_back({row, col});
                    if (count_domain(row, col, domains) == 0) return false;
                }
            }
        }
        return true;
    }

    void restore_constraints(int val, std::array<std::array<std::array<bool, 10>, 9>, 9>& domains,
                             const std::vector<std::pair<int, int>>& changes) {
        for (const auto& pos : changes) {
            domains[pos.first][pos.second][val] = true;
        }
    }

    int count_domain(int r, int c, const std::array<std::array<std::array<bool, 10>, 9>, 9>& domains) {
        int count = 0;
        for (int k = 1; k <= 9; ++k) {
            if (domains[r][c][k]) count++;
        }
        return count;
    }

    // Used for non-FC versions
    bool is_valid(int r, int c, int k) {
        for (int i = 0; i < 9; ++i) {
            if (grid[r][i] == k) return false;
            if (grid[i][c] == k) return false;
        }
        int start_row = 3 * (r / 3);
        int start_col = 3 * (c / 3);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (grid[start_row + i][start_col + j] == k) return false;
            }
        }
        return true;
    }

    bool solve_recursive(bool use_mrv, bool use_fc, 
                         std::array<std::array<std::array<bool, 10>, 9>, 9> domains) {
        int r, c;
        if (use_mrv) {
            if (!find_mrv_cell(r, c, domains, use_fc)) {
                solution = grid;
                return true;
            }
        } else {
            if (!find_empty_cell(r, c)) {
                solution = grid;
                return true;
            }
        }
        
        // Get possible values
        std::vector<int> possible_values;
        if (use_fc) { // Use reliable domain
            for (int k = 1; k <= 9; ++k) {
                if (domains[r][c][k]) {
                    possible_values.push_back(k);
                }
            }
        } else { // Recalculate
            for (int k = 1; k <= 9; ++k) {
                if (is_valid(r, c, k)) {
                    possible_values.push_back(k);
                }
            }
        }

        if (possible_values.empty() && grid[r][c] == 0) {
            stats.backtracks++;
            return false; 
        }

        for (int k : possible_values) {
            stats.decisions++;
            grid[r][c] = k;

            if (use_fc) {
                std::array<std::array<std::array<bool, 10>, 9>, 9> domains_copy = domains;
                std::vector<std::pair<int, int>> changes;
                
                if (propagate_constraints(r, c, k, domains_copy, changes)) {
                    if (solve_recursive(use_mrv, use_fc, domains_copy)) {
                        return true;
                    }
                }
            } else {
                 if (solve_recursive(use_mrv, use_fc, domains)) {
                    return true;
                }
            }

            stats.backtracks++;
            grid[r][c] = 0;
        }
        
        return false;
    }
};

// --- SAT Solver (DPLL) ---
class SudokuSATSolver {
public:
    int num_vars = 729;
    std::vector<std::vector<int>> clauses;
    std::vector<int> model; // 0=unassigned, 1=true, -1=false
    std::vector<double> vsids_scores;

    struct {
        long long decisions = 0;
        long long unit_props = 0;
        long long backtracks = 0;
    } stats;

    struct Heuristics {
        bool unit_prop = true;
        bool pure_literal = true;
        bool vsids = true;
    } config;

    SudokuSATSolver(const Grid& grid) {
        generate_clauses(grid);
        vsids_scores.resize(num_vars + 1, 0.0);
    }

    int to_var(int r, int c, int k) {
        return r * 81 + c * 9 + k;
    }

    void generate_clauses(const Grid& grid) {
        clauses.clear();
        auto add_clause = [this](const std::set<int>& lits) {
            clauses.push_back(std::vector<int>(lits.begin(), lits.end()));
        };

        // R1: Each cell has at least one digit
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                std::set<int> clause;
                for (int k = 1; k <= 9; ++k) clause.insert(to_var(r, c, k));
                add_clause(clause);
            }
        }

        // R2: Each cell has at most one digit
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                for (int k1 = 1; k1 <= 9; ++k1) {
                    for (int k2 = k1 + 1; k2 <= 9; ++k2) {
                        add_clause({-to_var(r, c, k1), -to_var(r, c, k2)});
                    }
                }
            }
        }

        // R3: Each row...
        for (int r = 0; r < 9; ++r) {
            for (int k = 1; k <= 9; ++k) {
                std::set<int> clause;
                for (int c = 0; c < 9; ++c) clause.insert(to_var(r, c, k));
                add_clause(clause);
                for(int c1=0; c1 < 9; ++c1)
                    for(int c2=c1+1; c2 < 9; ++c2)
                        add_clause({-to_var(r,c1,k), -to_var(r,c2,k)});
            }
        }
        
        // R4: Each column...
        for (int c = 0; c < 9; ++c) {
            for (int k = 1; k <= 9; ++k) {
                std::set<int> clause;
                for (int r = 0; r < 9; ++r) clause.insert(to_var(r, c, k));
                add_clause(clause);
                 for(int r1=0; r1 < 9; ++r1)
                    for(int r2=r1+1; r2 < 9; ++r2)
                        add_clause({-to_var(r1,c,k), -to_var(r2,c,k)});
            }
        }
        
        // R5: Each block...
        for (int br = 0; br < 3; ++br) {
            for (int bc = 0; bc < 3; ++bc) {
                for (int k = 1; k <= 9; ++k) {
                    std::set<int> clause;
                    for (int r = 0; r < 3; ++r)
                        for (int c = 0; c < 3; ++c)
                            clause.insert(to_var(br * 3 + r, bc * 3 + c, k));
                    add_clause(clause);

                     for (int r1 = 0; r1 < 3; ++r1)
                        for (int c1 = 0; c1 < 3; ++c1)
                            for (int r2 = 0; r2 < 3; ++r2)
                                for (int c2 = 0; c2 < 3; ++c2) {
                                    if (r1*3+c1 >= r2*3+c2) continue;
                                    add_clause({-to_var(br*3+r1, bc*3+c1, k), -to_var(br*3+r2, bc*3+c2, k)});
                                }
                }
            }
        }

        // R6: Pre-filled cells
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if (grid[r][c] != 0) {
                    add_clause({to_var(r, c, grid[r][c])});
                }
            }
        }
    }

    bool solve(Heuristics h) {
        config = h;
        stats = {0, 0, 0};
        model.assign(num_vars + 1, 0);
        vsids_scores.assign(num_vars + 1, 0.0);

        // Init VSIDS scores
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

    // --- BCP: Boolean Constraint Propagation ---
    // FIX: Added 'do_propagation' flag
    int bcp(std::vector<int>& trail, bool do_propagation) {
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

                if (!satisfied && false_count == clause.size()) {
                    return 0; // Conflict
                }
            }
            return 1; // No conflicts
        }
        
        // Normal BCP with propagation
        int head = 0;
        while(head < trail.size()) {
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
                if (false_count == clause.size()) {
                    return 0; // Conflict
                }
                if (false_count == clause.size() - 1 && unassigned_lit != 0) {
                    int var = std::abs(unassigned_lit);
                    if (model[var] == 0) {
                        stats.unit_props++;
                        model[var] = (unassigned_lit > 0) ? 1 : -1;
                        trail.push_back(unassigned_lit);
                    }
                }
            }
        }
        return 1;
    }
    
    // --- Pure Literal Elimination ---
    void pure_literal_assign(std::vector<int>& trail) {
        std::map<int, int> lit_counts; // 1=pos, -1=neg, 0=both
        for (const auto& clause : clauses) {
            bool satisfied = false;
            for(int lit : clause) if (model[std::abs(lit)] == ((lit > 0) ? 1 : -1)) satisfied = true;
            if(satisfied) continue;
            
            for(int lit : clause) {
                int var = std::abs(lit);
                if(model[var] != 0) continue;
                
                int val = (lit > 0) ? 1 : -1;
                if(lit_counts.find(var) == lit_counts.end()) {
                    lit_counts[var] = val;
                } else if (lit_counts[var] != val) {
                    lit_counts[var] = 0; // Both polarities seen
                }
            }
        }

        for(auto const& [var, val] : lit_counts) {
            if (val != 0 && model[var] == 0) {
                model[var] = val;
                trail.push_back(val * var);
            }
        }
    }

    int pick_branch_var() {
        if (config.vsids) {
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
        return 0; // All assigned
    }

    void decay_vsids_scores() {
        for (int var = 1; var <= num_vars; ++var) {
            vsids_scores[var] *= 0.95;
        }
    }

    bool dpll(std::vector<int>& trail) {
        int trail_head = trail.size();
        
        // FIX: Always call BCP to check for conflicts.
        // Pass the config flag to control propagation.
        if (bcp(trail, config.unit_prop) == 0) {
            stats.backtracks++;
            for (int i = trail_head; i < trail.size(); ++i) model[std::abs(trail[i])] = 0;
            trail.resize(trail_head);
            return false;
        }
        
        if (config.pure_literal) {
            pure_literal_assign(trail);
        }

        int var = pick_branch_var();
        if (var == 0) {
            return true; // All variables assigned, solution found
        }

        stats.decisions++;
        
        model[var] = 1; // Try True
        trail.push_back(var);
        if (dpll(trail)) return true;
        
        for (int i = trail_head; i < trail.size(); ++i) model[std::abs(trail[i])] = 0;
        trail.resize(trail_head);

        model[var] = -1; // Try False
        trail.push_back(-var);
        if (dpll(trail)) return true;

        for (int i = trail_head; i < trail.size(); ++i) model[std::abs(trail[i])] = 0;
        trail.resize(trail_head);
        
        if (config.vsids) {
            vsids_scores[var] += 1.0; // Bump score on conflict (simplified)
            if (stats.decisions % 100 == 0) decay_vsids_scores();
        }

        return false;
    }
    
    Grid get_solution_grid() {
        Grid solution_grid{};
        for (int var = 1; var <= num_vars; ++var) {
            if (model[var] == 1) {
                int r = (var - 1) / 81;
                int c = ((var - 1) % 81) / 9;
                int k = ((var - 1) % 9) + 1;
                solution_grid[r][c] = k;
            }
        }
        return solution_grid;
    }
};

// --- Main Benchmark ---
int main() {
    std::string puzzle_str = 
        "100007090"
        "030020008"
        "009600500"
        "005300900"
        "010080002"
        "600004000"
        "300000010"
        "040000007"
        "007000300";

    Grid puzzle = load_puzzle_from_string(puzzle_str);

    std::cout << std::string(70, '=') << std::endl;
    std::cout << "SUDOKU SOLVER BENCHMARK (C++)" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\nInitial Puzzle:" << std::endl;
    print_grid(puzzle);

    // --- Backtracking ---
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "BACKTRACKING SOLVER" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::vector<std::tuple<std::string, bool, bool>> bt_configs = {
        {"MRV + Forward Checking", true, true},
        {"MRV only", true, false},
        {"Forward Checking only", false, true},
        {"No heuristics", false, false},
    };

    struct BtResult { std::string name; double time; long long dec, back; bool success; };
    std::vector<BtResult> bt_results;

    for (const auto& [name, use_mrv, use_fc] : bt_configs) {
        BacktrackingSolver solver(puzzle);
        std::cout << "\n[" << name << "]" << std::endl;
        Timer t;
        bool solved = solver.solve(use_mrv, use_fc);
        double runtime = t.elapsed_s();
        
        if (solved) {
            std::cout << "  ✓ Solved in " << std::fixed << std::setprecision(6) << runtime << "s" << std::endl;
            std::cout << "    Decisions: " << solver.stats.decisions << ", Backtracks: " << solver.stats.backtracks << std::endl;
            bt_results.push_back({name, runtime, solver.stats.decisions, solver.stats.backtracks, true});
        } else {
            std::cout << "  ✗ Failed to solve" << std::endl;
            bt_results.push_back({name, runtime, 0, 0, false});
        }
    }
    std::cout << "\n  Solved Grid (Backtracking):" << std::endl;
    BacktrackingSolver temp_solver(puzzle);
    temp_solver.solve(true, true);
    print_grid(temp_solver.solution);


    // --- SAT Solver ---
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SAT SOLVER (DPLL)" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Create one solver just to get clause count
    SudokuSATSolver initial_solver(puzzle);
    std::cout << "Generating CNF clauses..." << std::endl;
    std::cout << "Generated " << initial_solver.clauses.size() << " clauses for " << initial_solver.num_vars << " variables." << std::endl;
    Grid solved_grid; // To store a valid solution
    bool sat_solution_found = false;

    // --- UPDATED: All configs are now active ---
    std::vector<std::pair<std::string, SudokuSATSolver::Heuristics>> sat_configs = {
        {"All heuristics", {true, true, true}},
        {"Unit Prop + VSIDS", {true, false, true}},
        {"Unit Prop + Pure Literal", {true, true, false}},
        {"Unit Propagation only", {true, false, false}},
        {"VSIDS only", {false, false, true}},
        {"Pure Literal only", {false, true, false}},
        {"No heuristics", {false, false, false}},
    };

    // --- UPDATED: Added 'timed_out' flag ---
    struct SatResult { 
        std::string name; 
        double time; 
        long long dec, props, back; 
        bool success; 
        bool timed_out;
    };
    std::vector<SatResult> sat_results;
    
    // --- FIX: Store futures in a vector to prevent blocking ---
    std::vector<std::future<bool>> sat_futures;

    // --- UPDATED: Loop with timeout logic ---
    for (const auto& [name, config] : sat_configs) {
        std::cout << "\n[" << name << "]" << std::endl;
        
        Timer t;
        double runtime = 0.0;
        bool solved = false;
        bool timed_out = false;
        
        // Create a thread-safe shared pointer for the solver
        auto sat_solver_ptr = std::make_shared<SudokuSATSolver>(puzzle); 
        auto stats_copy = sat_solver_ptr->stats; // To store final stats

        // Lambda captures the shared pointer *by value*
        auto solve_task = [sat_solver_ptr, config]() {
            return sat_solver_ptr->solve(config);
        };

        // --- FIX: Add future to vector ---
        sat_futures.emplace_back(std::async(std::launch::async, solve_task));
        std::chrono::seconds timeout_duration(30);

        // --- FIX: Wait on the *last* future added ---
        std::future_status status = sat_futures.back().wait_for(timeout_duration);

        if (status == std::future_status::timeout) {
            std::cout << "  ✗ TIMEOUT (exceeded 30 seconds)" << std::endl;
            timed_out = true;
            runtime = 30.0; // Record 30s as the time
        } else if (status == std::future_status::ready) {
            runtime = t.elapsed_s(); // Get total time
            try {
                // --- FIX: Get result from the *last* future ---
                solved = sat_futures.back().get(); // Get result (and any exceptions)
                stats_copy = sat_solver_ptr->stats; // Copy the final stats
                if (solved) {
                    std::cout << "  ✓ Solved in " << std::fixed << std::setprecision(6) << runtime << "s" << std::endl;
                    std::cout << "    Decisions: " << stats_copy.decisions 
                              << ", Unit Props: " << stats_copy.unit_props 
                              << ", Backtracks: " << stats_copy.backtracks << std::endl;
                    if (!sat_solution_found) {
                        solved_grid = sat_solver_ptr->get_solution_grid();
                        sat_solution_found = true;
                    }
                } else {
                    std::cout << "  ✗ Failed to solve (Unsatisfiable)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "  ✗ Error: " << e.what() << std::endl;
                solved = false;
            }
        }

        if (timed_out) {
            sat_results.push_back({name, runtime, 0, 0, 0, false, true});
        } else if (solved) {
            sat_results.push_back({name, runtime, stats_copy.decisions, stats_copy.unit_props, stats_copy.backtracks, true, false});
        } else {
            // Failed but did not time out
            sat_results.push_back({name, runtime, stats_copy.decisions, stats_copy.unit_props, stats_copy.backtracks, false, false});
        }
    }
    
    if (sat_solution_found) {
         std::cout << "\n  Solved Grid (SAT Solver):" << std::endl;
         print_grid(solved_grid);
    }

    // --- Summary ---
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SUMMARY" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << std::left;
    std::cout << "\nBacktracking Solver:" << std::endl;
    std::cout << std::setw(30) << "Configuration" << std::setw(15) << "Time (s)" << std::setw(15) << "Decisions" << "Backtracks" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    for(const auto& res : bt_results) {
        std::cout << std::setw(30) << res.name 
                  << std::setw(15) << std::fixed << std::setprecision(6) << res.time 
                  << std::setw(15) << res.dec << res.back << std::endl;
    }
    
    std::cout << "\nSAT Solver:" << std::endl;
    std::cout << std::setw(30) << "Configuration" << std::setw(15) << "Time (s)" << std::setw(15) << "Decisions" << std::setw(15) << "Unit Props" << "Backtracks" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    // --- UPDATED: Summary printing handles timeout ---
    for(const auto& res : sat_results) {
        if (res.timed_out) {
            std::cout << std::setw(30) << res.name 
                      << std::setw(15) << "> 30.000000"
                      << std::setw(15) << "N/A" << std::setw(15) << "N/A" << "N/A" << std::endl;
        } else if (res.success) {
            std::cout << std::setw(30) << res.name 
                      << std::setw(15) << std::fixed << std::setprecision(6) << res.time 
                      << std::setw(15) << res.dec << std::setw(15) << res.props << res.back << std::endl;
        } else { // FAILED (but didn't time out)
             std::cout << std::setw(30) << res.name 
                      << std::setw(15) << std::fixed << std::setprecision(6) << res.time 
                      << std::setw(15) << res.dec << std::setw(15) << res.props << res.back << " (FAILED)" << std::endl;
        }
    }
    
    return 0;
}


