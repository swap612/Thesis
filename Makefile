# Makefile to compile all c/cpp files in a directory
# @author Swapnil Raykar (swap612@gmail.com)

.PHONY: all clean

CC = gcc
# c source files
CSRCS:= $(wildcard *.c)
# cpp source files
CPPSRCS:= $(wildcard *.cpp) 

# cpp binary files
BINS:= ${CPPSRCS:%.cpp=%}
# c binary files
CBINS:= ${CSRCS:%.c=%}

all: ${BINS} ${CBINS}

%: %.o
	$(CC) $< -o $@

%.o: %.cpp  %.c
	$(CC) -c $<

clean:
	rm -rvf *.o ${BINS} ${CBINS}
