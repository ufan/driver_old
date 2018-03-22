# $ZEL: Makefile,v 1.3 2003/09/30 09:03:38 wuestner Exp $

CC           := gcc

DRIVER       := sis1100
IONAME       := $(shell echo $(DRIVER) | tr '[:lower:]' '[:upper:]')

WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations-Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat -Werror
#                -Wuninitialized
DRIVERBASE   := ../
DRIVERINCLUDE:= $(DRIVERBASE)
CPPFLAGS     := -I$(DRIVERINCLUDE) -I. -DDRIVER=$(DRIVER) -DIONAME=$(IONAME)
CFLAGS       := -g -ansi $(WFLAGS)

.PHONY: all
all: jtag_ids jtag_read jtag_write bin2mcs mcs2bin

jtag_ids: jtag_ids.c jtag_tools.c jtag_debug.c

jtag_read: jtag_read.c jtag_tools.c jtag_debug.c

jtag_write: jtag_write.c jtag_tools.c jtag_debug.c

clean:
	rm -f *.o core $(EXEC)
