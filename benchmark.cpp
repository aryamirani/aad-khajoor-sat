/**
 * @file benchmark.cpp
 * @brief Main benchmarking program for Sudoku solvers
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 * 
 * This program benchmarks both the backtracking and SAT-based Sudoku solvers
 * with various heuristic configurations to measure their impact on performance.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <tuple>
#include <future>
#include <chrono>
#include <memory>

#include "sudoku_types.h"
#include "backtracking_solver.h"
#include "sat_solver.h"
#include "timer.h"

/**
 * @brief Main entry point for the benchmarking program
 * 
 * Runs comprehensive tests on both solvers with multiple heuristic configurations,
 * measures performance, and generates a summary table.
 * 
 * @param argc Number of command-line arguments
 * @param argv Command-line arguments (argv[1] = puzzle string or file path)
 * @return int Exit code (0 for success)
 */
int main(int argc, char* argv[]) {
    // Show usage if help requested
    if (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "Usage: " << argv[0] << " [OPTIONS] [PUZZLE]\n\n"
                  << "OPTIONS:\n"
                  << "  --fast       Skip slow SAT configurations (recommended for hard puzzles)\n"
                  << "  -h, --help   Show this help message\n\n"
                  << "PUZZLE can be:\n"
                  << "  - 81-digit string (0 for empty cells)\n"
                  << "  - Path to a text file containing the puzzle\n"
                  << "  - Omit for default medium puzzle\n\n"
                  << "Examples:\n"
                  << "  " << argv[0] << "\n"
                  << "  " << argv[0] << " --fast test_cases/hard.txt\n"
                  << "  " << argv[0] << " test_cases/easy.txt\n"
                  << "  " << argv[0] << " \"530070000600195000...(81 digits)\"\n\n"
                  << "Note: SAT configs without Unit Propagation can be VERY slow (minutes to hours)\n";
        return 0;
    }
    
    bool fast_mode = false;
    std::string puzzle_str;
    int puzzle_arg_index = 1;
    
    // Check for --fast flag
    if (argc > 1 && std::string(argv[1]) == "--fast") {
        fast_mode = true;
        puzzle_arg_index = 2;
        std::cout << "⚡ Fast mode: Skipping slow SAT configurations" << std::endl;
    }
    
    if (argc > puzzle_arg_index) {
        // Check if argument is a file or direct puzzle string
        std::string arg = argv[puzzle_arg_index];
        if (arg.length() == 81 && arg.find_first_not_of("0123456789") == std::string::npos) {
            // Direct 81-digit puzzle string
            puzzle_str = arg;
            std::cout << "Using puzzle from command line." << std::endl;
        } else {
            // Try to load from file
            std::ifstream file(arg);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    // Skip comments and empty lines
                    if (line.empty() || line[0] == '#') continue;
                    puzzle_str += line;
                }
                file.close();
                // Extract only digits
                std::string cleaned;
                for (char c : puzzle_str) {
                    if (c >= '0' && c <= '9') cleaned += c;
                }
                puzzle_str = cleaned;
                if (puzzle_str.length() == 81) {
                    std::cout << "Loaded puzzle from file: " << arg << std::endl;
                } else {
                    std::cerr << "Error: File does not contain a valid 81-digit puzzle." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: Could not open file '" << arg << "'" << std::endl;
                return 1;
            }
        }
    } else {
        // Default medium puzzle
        puzzle_str = 
            "100007090"
            "030020008"
            "009600500"
            "005300900"
            "010080002"
            "600004000"
            "300000010"
            "040000007"
            "007000300";
        std::cout << "No puzzle provided. Using default medium puzzle." << std::endl;
    }
    
    if (puzzle_str.length() != 81) {
        std::cerr << "Error: Puzzle must be exactly 81 digits." << std::endl;
        return 1;
    }

    Grid puzzle = load_puzzle_from_string(puzzle_str);

    std::cout << std::string(70, '=') << std::endl;
    std::cout << "SUDOKU SOLVER BENCHMARK (C++)" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\nInitial Puzzle:" << std::endl;
    print_grid(puzzle);

    // ==============================================================
    // BACKTRACKING SOLVER BENCHMARKS
    // ==============================================================
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "BACKTRACKING SOLVER" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // Test configurations: (name, use_mrv, use_fc)
    std::vector<std::tuple<std::string, bool, bool>> bt_configs = {
        {"MRV + Forward Checking", true, true},
        {"MRV only", true, false},
        {"Forward Checking only", false, true},
        {"No heuristics", false, false},
    };

    struct BtResult { 
        std::string name; 
        double time; 
        long long dec, back; 
        bool success;
        bool timed_out;
    };
    std::vector<BtResult> bt_results;
    std::vector<std::future<bool>> bt_futures; // Store futures to prevent blocking

    for (const auto& [name, use_mrv, use_fc] : bt_configs) {
        std::cout << "\n[" << name << "]" << std::endl;
        
        Timer t;
        bool solved = false;
        double runtime = 0.0;
        
        auto solver_ptr = std::make_shared<BacktrackingSolver>(puzzle);
        
        auto solve_task = [solver_ptr, use_mrv, use_fc]() {
            return solver_ptr->solve(use_mrv, use_fc);
        };
        
        auto future = std::async(std::launch::async, solve_task);
        std::chrono::seconds timeout_duration(30);
        
        std::future_status status = future.wait_for(timeout_duration);
        runtime = t.elapsed_s();
        
        if (status == std::future_status::timeout) {
            std::cout << "  ✗ TIMEOUT (exceeded 30 seconds)" << std::endl;
            bt_results.push_back({name, 30.0, 0, 0, false, true});
            // Move future to storage to prevent blocking on destruction
            bt_futures.push_back(std::move(future));
        } else if (status == std::future_status::ready) {
            try {
                solved = future.get();
                if (solved) {
                    std::cout << "  ✓ Solved in " << std::fixed << std::setprecision(6) 
                              << runtime << "s" << std::endl;
                    std::cout << "    Decisions: " << solver_ptr->stats.decisions 
                              << ", Backtracks: " << solver_ptr->stats.backtracks << std::endl;
                    bt_results.push_back({name, runtime, solver_ptr->stats.decisions, 
                                         solver_ptr->stats.backtracks, true, false});
                } else {
                    std::cout << "  ✗ Failed to solve" << std::endl;
                    bt_results.push_back({name, runtime, 0, 0, false, false});
                }
            } catch (const std::exception& e) {
                std::cout << "  ✗ Error: " << e.what() << std::endl;
                bt_results.push_back({name, runtime, 0, 0, false, false});
            }
        }
    }
    
    // Display solved grid
    std::cout << "\n  Solved Grid (Backtracking):" << std::endl;
    BacktrackingSolver temp_solver(puzzle);
    temp_solver.solve(true, true);
    print_grid(temp_solver.solution);

    // ==============================================================
    // SAT SOLVER BENCHMARKS
    // ==============================================================
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SAT SOLVER (DPLL)" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Display CNF encoding information
    SudokuSATSolver initial_solver(puzzle);
    std::cout << "Generating CNF clauses..." << std::endl;
    std::cout << "Generated " << initial_solver.clauses.size() 
              << " clauses for " << initial_solver.num_vars << " variables." << std::endl;
    
    Grid solved_grid;
    bool sat_solution_found = false;

    // Test configurations: (name, heuristics)
    std::vector<std::pair<std::string, SudokuSATSolver::Heuristics>> sat_configs = {
        {"All heuristics", {true, true, true}},
        {"Unit Prop + VSIDS", {true, false, true}},
        {"Unit Prop + Pure Literal", {true, true, false}},
        {"Unit Propagation only", {true, false, false}},
    };
    
    // Add slow configurations only if not in fast mode
    if (!fast_mode) {
        sat_configs.push_back({"VSIDS only", {false, false, true}});
        sat_configs.push_back({"Pure Literal only", {false, true, false}});
        sat_configs.push_back({"No heuristics", {false, false, false}});
    }

    struct SatResult { 
        std::string name; 
        double time; 
        long long dec, props, back; 
        bool success;
        bool timed_out;
    };
    std::vector<SatResult> sat_results;
    std::vector<std::future<bool>> sat_futures; // Store futures to prevent blocking

    // Run SAT solver with 30-second timeout
    for (const auto& [name, config] : sat_configs) {
        std::cout << "\n[" << name << "]" << std::endl;
        
        Timer t;
        bool solved = false;
        double runtime = 0.0;
        
        auto sat_solver_ptr = std::make_shared<SudokuSATSolver>(puzzle);
        
        auto solve_task = [sat_solver_ptr, config]() {
            return sat_solver_ptr->solve(config);
        };
        
        auto future = std::async(std::launch::async, solve_task);
        std::chrono::seconds timeout_duration(30);
        
        std::future_status status = future.wait_for(timeout_duration);
        runtime = t.elapsed_s();
        
        if (status == std::future_status::timeout) {
            std::cout << "  ✗ TIMEOUT (exceeded 30 seconds)" << std::endl;
            sat_results.push_back({name, 30.0, 0, 0, 0, false, true});
            // Move future to storage to prevent blocking on destruction
            sat_futures.push_back(std::move(future));
        } else if (status == std::future_status::ready) {
            try {
                solved = future.get();
                if (solved) {
                    std::cout << "  ✓ Solved in " << std::fixed << std::setprecision(6) 
                              << runtime << "s" << std::endl;
                    std::cout << "    Decisions: " << sat_solver_ptr->stats.decisions 
                              << ", Unit Props: " << sat_solver_ptr->stats.unit_props 
                              << ", Backtracks: " << sat_solver_ptr->stats.backtracks << std::endl;
                    
                    if (!sat_solution_found) {
                        solved_grid = sat_solver_ptr->get_solution_grid();
                        sat_solution_found = true;
                    }
                    
                    sat_results.push_back({name, runtime, sat_solver_ptr->stats.decisions, 
                                          sat_solver_ptr->stats.unit_props, sat_solver_ptr->stats.backtracks, 
                                          true, false});
                } else {
                    std::cout << "  ✗ Failed to solve (Unsatisfiable)" << std::endl;
                    sat_results.push_back({name, runtime, sat_solver_ptr->stats.decisions, 
                                          sat_solver_ptr->stats.unit_props, sat_solver_ptr->stats.backtracks, 
                                          false, false});
                }
            } catch (const std::exception& e) {
                std::cout << "  ✗ Error: " << e.what() << std::endl;
                sat_results.push_back({name, runtime, 0, 0, 0, false, false});
            }
        }
    }
    
    if (sat_solution_found) {
         std::cout << "\n  Solved Grid (SAT Solver):" << std::endl;
         print_grid(solved_grid);
    }

    // ==============================================================
    // SUMMARY TABLE
    // ==============================================================
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SUMMARY" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << std::left;
    std::cout << "\nBacktracking Solver:" << std::endl;
    std::cout << std::setw(30) << "Configuration" 
              << std::setw(15) << "Time (s)" 
              << std::setw(15) << "Decisions" 
              << "Backtracks" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for(const auto& res : bt_results) {
        if (res.timed_out) {
            std::cout << std::setw(30) << res.name 
                      << std::setw(15) << "> 30.000000"
                      << std::setw(15) << "N/A" << "N/A (TIMEOUT)" << std::endl;
        } else {
            std::cout << std::setw(30) << res.name 
                      << std::setw(15) << std::fixed << std::setprecision(6) << res.time 
                      << std::setw(15) << res.dec 
                      << res.back << std::endl;
        }
    }
    
    std::cout << "\nSAT Solver:" << std::endl;
    std::cout << std::setw(30) << "Configuration" 
              << std::setw(15) << "Time (s)" 
              << std::setw(15) << "Decisions" 
              << std::setw(15) << "Unit Props" 
              << "Backtracks" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for(const auto& res : sat_results) {
        if (res.timed_out) {
            std::cout << std::setw(30) << res.name 
                      << std::setw(15) << "> 30.000000"
                      << std::setw(15) << "N/A" << std::setw(15) << "N/A" << "N/A (TIMEOUT)" << std::endl;
        } else if (res.success) {
            std::cout << std::setw(30) << res.name 
                      << std::setw(15) << std::fixed << std::setprecision(6) << res.time 
                      << std::setw(15) << res.dec 
                      << std::setw(15) << res.props << res.back << std::endl;
        } else {
            std::cout << std::setw(30) << res.name 
                      << std::setw(15) << std::fixed << std::setprecision(6) << res.time 
                      << std::setw(15) << res.dec 
                      << std::setw(15) << res.props 
                      << res.back << " (FAILED)" << std::endl;
        }
    }
    
    return 0;
}
