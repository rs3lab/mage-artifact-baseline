CXX = g++
CC = gcc
LDXX = g++
CXXFLAGS = -g -ggdb -O3 -march=native -Wall -Wextra -Izipf/
LDFLAGS = -lpthread 
OBJ = seq_scan.o  zipf.o rand.o helper.o

default: all

zipf.o: zipf/zipf.c
	$(CC) $(CXXFLAGS) -o $@ -c $<

rand.o: zipf/rand.c
	$(CC) $(CXXFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

seq_scan: seq_scan.o zipf.o rand.o
	$(LDXX) -o $@ $^ $(LDFLAGS)

.PHONY: all clean default

all: seq_scan

clean:
	rm -f *.o seq_scan


