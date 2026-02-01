# ============================================
#  Grow Castle Progress Tracker - Makefile
# ============================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
CFLAGS_DEBUG = -Wall -Wextra -Werror -pedantic -std=c11 -g -fsanitize=address,undefined -fno-omit-frame-pointer

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
DATA_DIR = data

# Executable name
TARGET = $(BIN_DIR)/grow_castle_tool
TARGET_DEBUG = $(BIN_DIR)/grow_castle_tool_debug

# Find all .c files automatically
SRC = $(wildcard $(SRC_DIR)/*.c)

# Object files (in bin/)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)
OBJ_DEBUG = $(SRC:$(SRC_DIR)/%.c=$(BIN_DIR)/%_debug.o)

# === Default rule ===
all: setup $(TARGET)

# Build release target
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ -lm
	@echo ""
	@echo " Build complete: $(TARGET)"
	@echo "Run with: make run"
	@echo ""

# Build debug target with sanitizers
$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(CFLAGS_DEBUG) $(OBJ_DEBUG) -o $@ -lm
	@echo ""
	@echo " Debug build complete: $(TARGET_DEBUG)"
	@echo "Run with: make run-debug"
	@echo ""

# Compile source files (release)
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile source files (debug)
$(BIN_DIR)/%_debug.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS_DEBUG) -I$(INC_DIR) -c $< -o $@

# Create directories if not exist
setup:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(DATA_DIR)

# Run program (release)
run: all
	@echo " Running Grow Castle Progress Tracker..."
	@./$(TARGET)

# Run program (debug with sanitizers)
run-debug: debug
	@echo " Running Grow Castle Progress Tracker (Debug with AddressSanitizer)..."
	@./$(TARGET_DEBUG)

# Build debug version
debug: setup $(TARGET_DEBUG)

# Compile and check for issues (debug build)
check: debug
	@echo " Debug build successful! Running with sanitizers enabled..."
	@./$(TARGET_DEBUG) 2>&1 || true

# Clean build files
clean:
	@echo " Cleaning build files..."
	@rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*_debug.o $(TARGET) $(TARGET_DEBUG)
	@echo "Done."

# Full reset (also clears saved data)
reset:
	@echo " Full reset: removing builds and data..."
	@rm -rf $(BIN_DIR)/*.o $(BIN_DIR)/*_debug.o $(TARGET) $(TARGET_DEBUG) $(DATA_DIR)/*.csv
	@echo "All data cleared."

# Help command
help:
	@echo "Available commands:"
	@echo "  make              - compile everything (release)"
	@echo "  make run          - compile and run program (release)"
	@echo "  make debug        - compile debug version with sanitizers"
	@echo "  make run-debug    - compile and run debug version"
	@echo "  make check        - build debug and run with sanitizers"
	@echo "  make clean        - remove object and binary files"
	@echo "  make reset        - clean + remove saved data"
	@echo "  make help         - show this help message"

.PHONY: all run debug run-debug check clean reset help setup