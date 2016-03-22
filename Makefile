CC = g++
CFLAGS = -std=c++11 -Wextra -Wall
TAR = tar
TARFLAGS = cvf
TARNAME = ex2.tar
TARSRCS = uthreads.cpp Thread.cpp Thread.h Makefile README

all: uthreads.o
	ar rcs libuthreads.a uthreads.o Thread.o

uthreads.o: uthreads.cpp uthreads.h Thread.cpp Thread.h
	$(CC) $(CFLAGS) uthreads.cpp -c &&\
	$(CC) $(CFLAGS) Thread.cpp -c



clean:
	rm uthreads.o libuthreads.a ex2.tar

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

.PHONY: clean, all, tar
