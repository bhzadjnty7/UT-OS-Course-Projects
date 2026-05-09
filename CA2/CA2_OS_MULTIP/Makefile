CC = g++
CFLAGS = -std=c++11 -Wall -O2

all: main extract_transform load compute output

main: main.cpp 
	$(CC) $(CFLAGS) -o main main.cpp

extract_transform: extract_transform.cpp 
	$(CC) $(CFLAGS) -o extract_transform extract_transform.cpp

load: load.cpp 
	$(CC) $(CFLAGS) -o load load.cpp

compute: compute.cpp 
	$(CC) $(CFLAGS) -o compute compute.cpp

output: output.cpp 
	$(CC) $(CFLAGS) -o output output.cpp

clean:
	rm -f main extract_transform load compute output GameRanking.csv