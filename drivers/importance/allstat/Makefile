#
## Makefile by Po-Hsien Tseng <r00922073@csie.ntu.edu.tw>
#
obj-m       += allstat.o
obj-m		+= allstat_with_name.o

KDIR=/home/phtseng/Desktop/proc-connector-extended-sgs3
#KDIR=/usr/src/linux-3.9.7-1-ARCH
ARCH=arm
CROSS_COMPILE=arm-eabi-

all:
#	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean
#	$(MAKE) -C $(KDIR) M=$(PWD) clean

push:
	adb push allstat.ko /sdcard/
	adb push allstat_with_name.ko /sdcard/
