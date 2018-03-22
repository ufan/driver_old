# $ZEL: Makefile,v 1.6 2008/11/25 15:53:54 wuestner Exp $

DRIVER       := sis1100
IONAME       := $(shell echo $(DRIVER) | tr '[:lower:]' '[:upper:]')

CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat \
                -Werror
#               -Wuninitialized

DRIVERBASE   := ../
DRIVERINCLUDE:= $(DRIVERBASE)/dev/pci
CPPFLAGS     := -I$(DRIVERINCLUDE) -I. -DDRIVER=$(DRIVER) -DIONAME=$(IONAME)
CFLAGS       := -g -ansi $(WFLAGS)

EXEC         := read_eeprom verify_eeprom write_eeprom

.PHONY: all
all: $(EXEC)

read_eeprom: read_eeprom.c eeprom_tools.c eeprom_names.c

verify_eeprom: verify_eeprom.c eeprom_tools.c eeprom_names.c

write_eeprom: write_eeprom.c eeprom_tools.c eeprom_names.c

clean:
	rm -f *.o core $(EXEC)
