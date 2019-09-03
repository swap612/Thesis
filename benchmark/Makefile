# Makefile to compile all c/cpp files in a directory
# @author Swapnil Raykar (swap612@gmail.com)

.PHONY: all clean

CC = g++

# c source files
CSRCS:= $(wildcard *.c)
# cpp source files
CPPSRCS:= $(wildcard *.cpp) 

# cpp binary files
BINS:= ${CPPSRCS:%.cpp=%}
# c binary files
CBINS:= ${CSRCS:%.c=%}

all: ${BINS} ${CBINS}

%: %.cpp 
	$(CC) -std=c++11 $< -o $@

clean:
	rm -rvf *.o ${BINS} ${CBINS}
