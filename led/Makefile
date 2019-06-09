
LINUX_SRC_PATH="/home/toni/programming/yocto/beaglebone-x15-yocto/poky/build/tmp/work/am57xx_evm-poky-linux-gnueabi/linux-xenomai/4.14.71+3.0.8-r0/linux-am57xx_evm-standard-build"
YOCTO_SDK_ENV_FILE="/opt/poky/2.6.1/environment-setup-armv7at2hf-neon-poky-linux-gnueabi"


ifneq ($(KERNELRELEASE),)
	obj-m := led.o

else
	KERNELDIR ?= ${LINUX_SRC_PATH}
	PWD := $(shell pwd)
	
default:
	source ${YOCTO_SDK_ENV_FILE}
	$(MAKE) ARCH=${ARCH} CROSS_COMPILE=${TARGET_PREFIX} -C $(KERNELDIR) M=$(PWD) modules

endif
