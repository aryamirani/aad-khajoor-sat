# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -I./include
LDFLAGS = -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = .

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/sudoku_benchmark

# Main build target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS) $(BUILD_DIR)/benchmark.o | $(BIN_DIR)
	@echo "Linking $@..."
	@$(CXX) $(OBJECTS) $(BUILD_DIR)/benchmark.o -o $@ $(LDFLAGS)
	@echo "✓ Build successful! Run with: ./sudoku_benchmark"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile benchmark main
$(BUILD_DIR)/benchmark.o: benchmark.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Create bin directory
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "✓ Clean complete"

# Run all tests
test: $(TARGET)
	@echo "Running test suite..."
	@bash scripts/run_all_tests.sh

# Display help
help:
	@echo "SAT Solver & Sudoku Benchmark - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the project"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make test     - Build and run all tests"
	@echo "  make help     - Display this help message"

.PHONY: all clean test help
