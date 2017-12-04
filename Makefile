obj-m := project.o
project-objs := coop-iosched.o
CONFIG_MODULE_SIG = n
#KDIR=~/linux
KDIR= /lib/modules/$(shell uname -r)/build
alli: coop-iosched.c
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean
