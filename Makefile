# ============================================
#  Grow Castle Progress Tracker - Makefile
# ============================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
DATA_DIR = data

# Executable name
TARGET = $(BIN_DIR)/grow_castle_tool

# Find all .c files automatically
SRC = $(wildcard $(SRC_DIR)/*.c)

# Object files (in bin/)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

# === Default rule ===
all: setup $(TARGET)

# Build target
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@
	@echo ""
	@echo " Build complete: $(TARGET)"
	@echo "Run with: make run"
	@echo ""

# Compile source files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Create directories if not exist
setup:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(DATA_DIR)

# Run program
run: all
	@echo " Running Grow Castle Progress Tracker..."
	@./$(TARGET)

# Clean build files
clean:
	@echo " Cleaning build files..."
	@rm -f $(BIN_DIR)/*.o $(TARGET)
	@echo "Done."

# Full reset (also clears saved data)
reset:
	@echo " Full reset: removing builds and data..."
	@rm -rf $(BIN_DIR)/.o $(TARGET) $(DATA_DIR)/.txt
	@echo "All data cleared."

# Help command
help:
	@echo "Available commands:"
	@echo "  make           - compile everything"
	@echo "  make run       - compile and run program"
	@echo "  make clean     - remove object and binary files"
	@echo "  make reset     - clean + remove saved data"
	@echo "  make help      - show this help message"

.PHONY: all run clean reset help setup