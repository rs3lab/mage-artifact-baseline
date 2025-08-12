CXX = g++
CC = gcc
LDXX = g++
CXXFLAGS = -g -ggdb -O3 -march=native -Wall -Wextra -Izipf/
LDFLAGS = -lpthread 
OBJ = page_fault.o mlock.o zipf.o rand.o helper.o

default: all

zipf.o: zipf/zipf.c
	$(CC) $(CXXFLAGS) -o $@ -c $<

rand.o: zipf/rand.c
	$(CC) $(CXXFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

mage_noeviction: mage_noeviction.o helper.o
	$(LDXX) -o $@ $^ $(LDFLAGS)

page_fault: page_fault.o zipf.o rand.o
	$(LDXX) -o $@ $^ $(LDFLAGS)

workingset: workingset.o
	$(LDXX) -o $@ $< $(LDFLAGS)

mlock: mlock.o
	$(LDXX) -o $@ $< $(LDFLAGS)

.PHONY: all clean default

all: page_fault workingset mlock mage_noeviction

clean:
	rm -f *.o page_fault mlock


