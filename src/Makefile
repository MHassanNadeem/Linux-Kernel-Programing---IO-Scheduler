obj-m := coop.o
coop-objs := coop-iosched.o
CONFIG_MODULE_SIG = n
KDIR= /lib/modules/$(shell uname -r)/build


alli: coop-iosched.c
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean

insert:
	sudo insmod coop.ko

remove:
	sudo rmmod coop