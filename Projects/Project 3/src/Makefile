obj-m := mtest.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	rm *.o *.ko *.mod.c *.mod Module.symvers modules.order -f
