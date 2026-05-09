# Compiler name
CXX = g++

# Compiler options
CXXFLAGS = -Wall -O2

# Object files for server and client
OBJS_SERVER = server_final.o
OBJS_CLIENT = client_final.o

# Default target: build both executables
all: server_final client_final

# Build server executable
server_final: $(OBJS_SERVER)
	$(CXX) $(CXXFLAGS) -o server_final $(OBJS_SERVER)

# Build client executable
client_final: $(OBJS_CLIENT)
	$(CXX) $(CXXFLAGS) -o client_final $(OBJS_CLIENT)

# General rule to build .o from .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean temporary files
clean:
	rm -f *.o server_final client_final