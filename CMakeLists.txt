cmake_minimum_required(VERSION 3.10)
project(SudokuSATSolver VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/sudoku_types.cpp
    src/backtracking_solver.cpp
    src/sat_solver.cpp
    src/timer.cpp
)

# Benchmark executable
add_executable(sudoku_benchmark 
    ${SOURCES}
    benchmark.cpp
)

# Enable optimizations for Release build
target_compile_options(sudoku_benchmark PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g>
)

# Link threading library for std::async
find_package(Threads REQUIRED)
target_link_libraries(sudoku_benchmark Threads::Threads)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
