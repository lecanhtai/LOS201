KDIR ?= $(HOME)/embedded_lab1/kernel/linux-5.15
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

obj-m += sms_sensor_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) KCFLAGS="-march=armv7-a -mfpu=neon-vfpv3 -mfloat-abi=hard" modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
