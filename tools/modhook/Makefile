KERNDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(shell pwd)
RM = rm -f

obj-m := modhook.o

.PHONY: all modhook clean

all: modhook

modhook: modhook.c modhook.h
#	make -C $(KERNDIR) SUBDIRS=$(BUILD_DIR) KBUILD_VERBOSE=$(VERBOSE) modules
	make -C $(KERNDIR) M=$(PWD) modules

clean:
	make -C $(KERNDIR) M=$(PWD) clean
	