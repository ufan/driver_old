# $ZEL: Makefile,v 1.1 2009/04/13 00:13:30 wuestner Exp $

CC           := gcc
WFLAGSg       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat -Werror
WFLAGSo       := $(WFLAGSg) -Wuninitialized

DRIVERBASE   := ../..
DRIVERINCLUDE:= $(DRIVERBASE)/dev/pci
CPPFLAGS     := -I$(DRIVERINCLUDE) -I.
#CFLAGS       := -O -ansi $(WFLAGSo)
CFLAGS       := -g -ansi $(WFLAGSg)

EXEC         := read_transparent stop_transparent

.PHONY: all
all: $(EXEC)

clean:
	rm -f *.o core $(EXEC)
