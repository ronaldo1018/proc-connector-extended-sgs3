#
## Makefile by Po-Hsien Tseng <r00922073@csie.ntu.edu.tw>
#
obj-m       += timer_connector.o
KDIR=/home/phtseng/Desktop/proc-connector-extended-sgs3
ARCH=arm
CROSS_COMPILE=arm-eabi-

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean

push:
	adb push timer_connector.ko /sdcard/
