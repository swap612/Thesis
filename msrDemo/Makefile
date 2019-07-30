# Makefile to compile the linux modules
# @author Swapnil Raykar <swap612@gmail.com>

obj-m += tsc.o

all:
	make -C /lib/modules/` uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
