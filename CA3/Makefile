# Makefile for Neural Network Project

CC = g++
CFLAGS = -Wall -g -O3 -std=c++11
LIBS = -lpthread -lm

# Target binary
TARGET = parallel_nn

# Source files
SRC = parallel_nn.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Default target
all: $(TARGET)

# Compile the program
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile source files to object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJ) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Serial version for comparison
serial: serial.cpp
	$(CC) $(CFLAGS) -o serial_nn serial.cpp -lm
	
# Run serial version
run_serial: serial
	./serial_nn

# Compare performance between serial and parallel versions
compare: serial $(TARGET)
	@echo "Running serial version..."
	time ./serial_nn
	@echo "\nRunning parallel version..."
	time ./$(TARGET)
	@echo "\nComparison complete!"

.PHONY: all clean run serial run_serial compare