obj-m :=list_all_process.o
KDIR :=/lib/modules/$(shell uname -r)/build
PWD :=$(shell pwd)
default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(DKIR) M=$(PWD) clean
