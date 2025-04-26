# Compiler settings
CC = gcc
CFLAGS = -Wall -g

# Directories
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin

# Files
COMMON_H = common.h
MAIN_SRC = main.c
TRACKER_SRC = tracker.c
PEER_SRC = peer.c

# Object files
MAIN_OBJ = $(OBJ_DIR)/main.o
TRACKER_OBJ = $(OBJ_DIR)/tracker.o
PEER_OBJ = $(OBJ_DIR)/peer.o

# Output executables
EXEC = $(BIN_DIR)/os_program

# Targets
all: $(EXEC)

# Rule to compile main
$(MAIN_OBJ): $(MAIN_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -c $(MAIN_SRC) -o $(MAIN_OBJ)

# Rule to compile tracker
$(TRACKER_OBJ): $(TRACKER_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -c $(TRACKER_SRC) -o $(TRACKER_OBJ)

# Rule to compile peer
$(PEER_OBJ): $(PEER_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -c $(PEER_SRC) -o $(PEER_OBJ)

# Rule to link the final executable
$(EXEC): $(MAIN_OBJ) $(TRACKER_OBJ) $(PEER_OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(MAIN_OBJ) $(TRACKER_OBJ) $(PEER_OBJ)

# Clean up object and binary files
clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/*

# Create necessary directories if they don't exist
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Add phony targets (non-file targets)
.PHONY: all clean
