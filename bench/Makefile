CC=gcc
CXX=g++
LD=g++
CFLAGS=-O2 -g -Wall -std=gnu11
CXXFLAGS=-O2 -g -Wall -std=gnu++11
LDFLAGS=-L../grisu -L../gay -L../errol -static -ldouble-conversion -lerrol -lgay
#LDFLAGS=-L../grisu -L../gay -L../errol -static -lerrol -lgay

all: bench

bench: src/main.o src/avltree.o src/integer.o src/search.o ../errol/liberrol.a
	$(LD) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
