/**
 * @file sat_solver.h
 * @brief DPLL-based SAT solver for Sudoku encoded as CNF
 * @authors Arya Mirani, Ujjwal
 * @date December 2025
 */

#ifndef SAT_SOLVER_H
#define SAT_SOLVER_H

#include <vector>
#include <set>
#include <map>
#include "sudoku_types.h"

/**
 * @class SudokuSATSolver
 * @brief Solves Sudoku puzzles by encoding them as SAT problems and using DPLL
 * 
 * This class demonstrates the universality of SAT by reducing the Sudoku problem
 * to Boolean satisfiability. The solver uses the DPLL (Davis-Putnam-Logemann-Loveland)
 * algorithm with several optimization heuristics.
 * 
 * **Encoding Scheme:**
 * - 729 variables: One for each (row, column, digit) combination
 * - Variable naming: V(r,c,k) = "cell (r,c) contains digit k"
 * - Variable index: r*81 + c*9 + k (where r,c,k ∈ [0,8])
 * 
 * **CNF Constraints (11,988 clauses):**
 * - R1: Each cell has at least one digit (81 clauses)
 * - R2: Each cell has at most one digit (2,916 clauses)
 * - R3: Each row contains each digit exactly once (810 clauses)
 * - R4: Each column contains each digit exactly once (810 clauses)
 * - R5: Each 3×3 block contains each digit exactly once (810 clauses)
 * - R6: Pre-filled cells are fixed (varies by puzzle)
 * 
 * **DPLL Algorithm:**
 * 1. **Unit Propagation:** Force assignments for unit clauses
 * 2. **Pure Literal Elimination:** Assign pure literals (optional)
 * 3. **Variable Selection:** Choose unassigned variable (using VSIDS if enabled)
 * 4. **Recursive Search:** Try both true/false and backtrack on conflicts
 * 
 * **Heuristics:**
 * - **Unit Propagation:** Deterministic simplification, crucial for performance
 * - **Pure Literal:** Eliminates variables appearing in only one polarity
 * - **VSIDS:** Activity-based branching with conflict-driven score bumping
 * 
 * @note All algorithms implemented from scratch without external SAT libraries
 * @note Performance varies significantly based on heuristic configuration
 */
class SudokuSATSolver {
public:
    /// Total number of Boolean variables (729 for 9×9 Sudoku)
    int num_vars = 729;
    
    /// CNF formula as list of clauses (each clause is a disjunction of literals)
    std::vector<std::vector<int>> clauses;
    
    /// Current variable assignments: 0=unassigned, 1=true, -1=false
    std::vector<int> model;
    
    /// VSIDS activity scores for variable selection heuristic
    std::vector<double> vsids_scores;
    
    /**
     * @struct Statistics
     * @brief Performance metrics collected during SAT solving
     */
    struct {
        long long decisions = 0;    ///< Number of branching decisions made
        long long unit_props = 0;   ///< Number of unit propagations performed
        long long backtracks = 0;   ///< Number of times backtracking occurred
    } stats;
    
    /**
     * @struct Heuristics
     * @brief Configuration flags for DPLL heuristics
     */
    struct Heuristics {
        bool unit_prop = true;      ///< Enable unit propagation (BCP)
        bool pure_literal = true;   ///< Enable pure literal elimination
        bool vsids = true;          ///< Enable VSIDS branching heuristic
    } config;
    
    /**
     * @brief Constructs a SAT solver and generates CNF encoding for the puzzle
     * 
     * @param grid Initial Sudoku puzzle with 0 representing empty cells
     * 
     * Automatically generates all CNF clauses (R1-R6) and initializes data structures.
     * The puzzle is encoded using one-hot encoding for cell values.
     * 
     * Time Complexity: O(n³) where n=9 for clause generation
     * Space Complexity: O(c) where c≈12,000 clauses
     */
    explicit SudokuSATSolver(const Grid& grid);
    
    /**
     * @brief Solves the SAT instance using DPLL with specified heuristics
     * 
     * @param h Heuristics configuration (unit_prop, pure_literal, vsids flags)
     * 
     * @return bool True if satisfiable (puzzle solvable), false if unsatisfiable
     * 
     * @note If satisfiable, the solution can be retrieved with get_solution_grid()
     * @note Statistics are accumulated in the `stats` member
     * 
     * Time Complexity: O(2^n) worst case, significantly improved by heuristics
     * Space Complexity: O(n + c) for model and clauses
     */
    bool solve(Heuristics h);
    
    /**
     * @brief Converts SAT solution back to Sudoku grid
     * 
     * Extracts the solved Sudoku grid from the SAT model by checking which
     * variables are assigned true.
     * 
     * @return Grid The solved 9×9 Sudoku puzzle
     * 
     * @pre solve() must have returned true (satisfiable)
     * @warning Returns invalid grid if called before successful solve()
     * 
     * Time Complexity: O(n²) where n=9
     * Space Complexity: O(1)
     */
    Grid get_solution_grid();
    
private:
    /**
     * @brief Converts (row, col, digit) to SAT variable index
     * 
     * Maps Sudoku cell and digit to unique variable number using formula:
     * var = row*81 + col*9 + digit
     * 
     * @param r Row index (0-8)
     * @param c Column index (0-8)
     * @param k Digit value (1-9)
     * 
     * @return int Variable index (1-729)
     * 
     * Time Complexity: O(1)
     */
    int to_var(int r, int c, int k);
    
    /**
     * @brief Generates all CNF clauses for Sudoku constraints
     * 
     * Creates clauses for rules R1-R6:
     * - At-least-one and at-most-one constraints for cells
     * - Row, column, and block uniqueness constraints
     * - Pre-filled cell constraints
     * 
     * @param grid Initial puzzle to encode
     * 
     * Time Complexity: O(n³) where n=9
     * Space Complexity: O(c) where c≈12,000 clauses
     */
    void generate_clauses(const Grid& grid);
    
    /**
     * @brief Boolean Constraint Propagation (Unit Propagation)
     * 
     * Core DPLL subroutine that:
     * 1. Identifies unit clauses (only one unassigned literal)
     * 2. Forces those literals to satisfy the clause
     * 3. Propagates implications iteratively
     * 4. Detects conflicts (empty clauses)
     * 
     * @param trail List of assigned literals (grows during propagation)
     * @param do_propagation If false, only checks conflicts without propagating
     * 
     * @return int 1 if successful, 0 if conflict detected
     * 
     * @note Updates `model` and `trail` during propagation
     * @note Increments `stats.unit_props` counter
     * 
     * Time Complexity: O(c·l) where c=clauses, l=literals per clause
     * Space Complexity: O(1) - modifies parameters in-place
     */
    int bcp(std::vector<int>& trail, bool do_propagation);
    
    /**
     * @brief Pure Literal Elimination heuristic
     * 
     * Identifies variables that appear only in positive or only in negative form
     * across all unsatisfied clauses, and assigns them to satisfy those clauses.
     * 
     * Pure literals can be safely assigned without causing conflicts since they
     * don't appear in both polarities.
     * 
     * @param trail List of assigned literals (pure literals are appended)
     * 
     * @note Modifies `model` and `trail` with pure literal assignments
     * @note Only examines clauses not already satisfied
     * 
     * Time Complexity: O(c·l) where c=clauses, l=literals per clause
     * Space Complexity: O(v) for tracking polarities, v=variables
     */
    void pure_literal_assign(std::vector<int>& trail);
    
    /**
     * @brief Selects next unassigned variable for branching
     * 
     * Uses VSIDS (Variable State Independent Decaying Sum) if enabled,
     * otherwise falls back to first-unassigned selection.
     * 
     * VSIDS prioritizes variables involved in recent conflicts, which
     * often leads to faster conflict detection.
     * 
     * @return int Variable index to branch on (1-729), or 0 if all assigned
     * 
     * Time Complexity: O(v) where v=729 variables
     * Space Complexity: O(1)
     */
    int pick_branch_var();
    
    /**
     * @brief Decays VSIDS scores for all variables
     * 
     * Multiplies all activity scores by 0.95 to give more weight to recent
     * conflicts. Called periodically during search.
     * 
     * Time Complexity: O(v) where v=729
     * Space Complexity: O(1)
     */
    void decay_vsids_scores();
    
    /**
     * @brief Recursive DPLL search implementation
     * 
     * Core SAT solving algorithm:
     * 1. Perform BCP (unit propagation)
     * 2. Apply pure literal elimination
     * 3. Check for completion (all variables assigned)
     * 4. Select variable and try both polarities
     * 5. Backtrack on conflicts
     * 
     * @param trail Current assignment trail (decisions + propagations)
     * 
     * @return bool True if satisfiable, false if unsatisfiable
     * 
     * @note Updates `stats` counters (decisions, backtracks)
     * @note Modifies `model` during search
     * @note Trail is restored on backtracking
     * 
     * Time Complexity: O(2^v) worst case, v=729 variables
     * Space Complexity: O(v) for recursion depth
     */
    bool dpll(std::vector<int>& trail);
};

#endif // SAT_SOLVER_H
