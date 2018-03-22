# $ZEL: Makefile,v 1.12 2016/03/16 20:29:53 wuestner Exp $
#
# We're just setting the stage here and
# let the Kernel Makefile do the real work.
#

KERNVER := $(shell uname -r)
MODULENAME := sis1100


#16NOV2016 For FEDORA23_Kernel4.2.3.300
 ccflags-y  = -I/usr/src/kernels/$(KERNVER)/include/linux

#04APR2017 For UBUNTU16.04.1_Kernel4.8.0-36
 ccflags-y += -I/usr/src/linux-headers-$(KERNVER)/include/linux

#01NOV2017 For DEBIAN
 ccflags-y += -I/usr/src/linux-headers-$(shell uname -r | grep -Po '(.*)-')common/include/linux/

.PHONY: driver

driver:
	make -C /lib/modules/$(KERNVER)/build SUBDIRS=$(shell pwd) modules

install:
	-/sbin/rmmod $(MODULENAME)
	make -C /lib/modules/$(KERNVER)/build SUBDIRS=$(shell pwd) modules_install
	/sbin/depmod -a
	/sbin/modprobe $(MODULENAME)
	cp -f ../../udev-rules/92-struck.rules /etc/udev/rules.d/92-struck.rules

clean:
	-rm -f *.o *.ko .*.o.cmd .*.ko.cmd *.mod.c Module.symvers modules.order
	-rm -f Module.markers
	-rm -r -f .tmp_versions


ifndef src
    src:=$(shell pwd)
endif

SOURCES:=$(notdir $(wildcard $(src)/*.c))
sis1100-objs := $(patsubst %.c, %.o, $(filter-out %netbsd.c, $(SOURCES)))

XVER    := X${KERNVER}
# accepted kernel versions
V24 := X2.4
V26 := X2.6
V3 := X3.
V4 := X4.

ifneq (,$(or $(findstring ${V26},${XVER}),$(findstring ${V3},${XVER}),$(findstring ${V4},${XVER})))

    # kernel version 2.6 or later
    $(info kernel version ${KERNVER})
    EXTRA_CFLAGS += -DMODULENAME=$(MODULENAME)
    obj-m += sis1100.o

else ifneq (,$(findstring ${V24},${XVER}))

    # kernel version 2.4.*; does probably not work anymore
    $(warning old kernel version ${KERNVER})
modules: sis1100.o
sis1100.o: $(sis1100-objs)
	$(LD) -r -o $@ $^

else 
    $(error unsupported kernel version ${KERNVER})
endif
