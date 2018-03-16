# $ZEL: Makefile,v 1.1 2009/04/13 00:13:30 wuestner Exp $

CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat \
                -Werror -Wuninitialized

DRIVERBASE   := ../..
DRIVERINCLUDE:= $(DRIVERBASE)/dev/pci
CPPFLAGS     := -I$(DRIVERINCLUDE) -I.
CFLAGS       := -O -ansi $(WFLAGS)

EXEC         := irq_link_signal irq_link_wait irq_link_select irq_link_poll \
                list_sis1100

.PHONY: all
all: $(EXEC)

clean:
	rm -f *.o core $(EXEC)
