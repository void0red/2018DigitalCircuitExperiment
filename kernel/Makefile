ifneq  ($(KERNELRELEASE),)
obj-m:=irq_stop.o
else
KDIR := /lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
all:
    make -C $(KDIR) M=$(PWD) modules
clean:
    rm -f *.ko *.o *.symvers *.cmd *.cmd.o
endif