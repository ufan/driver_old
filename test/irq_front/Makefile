# $ZEL: Makefile,v 1.6 2008/11/25 15:53:54 wuestner Exp $

CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat \
                -Werror -Wuninitialized

DRIVERBASE   := ../..
DRIVERINCLUDE:= $(DRIVERBASE)/dev/pci
CPPFLAGS     := -I$(DRIVERINCLUDE) -I.
CFLAGS       := -O -ansi $(WFLAGS)

EXEC         := irq_front_signal irq_front_wait irq_front_select \
                irq_front_poll front_io

.PHONY: all
all: $(EXEC)

clean:
	rm -f *.o core $(EXEC)
