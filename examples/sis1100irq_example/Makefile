# DRIVERBASE should point to the base directory of the driver.
# The needed include file (sis1100_var.h) can then be found as
# "dev/pci/sis1100_var.h"
DRIVERBASE   := ../..

# all other definitions should work without change

srcdir = .

#CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat -Wuninitialized -Werror

CPPFLAGS     := -I$(DRIVERBASE) -I.
CFLAGS       := -O $(WFLAGS)

cfiles       := $(notdir $(wildcard $(srcdir)/*.c))
#cfiles       := link_poll.c
EXEC         := $(cfiles:.c=)

.PHONY: all
all: $(EXEC)

clean:
	rm -f *.o core $(EXEC)

realclean:  clean
	cp Makefile Makefile.bak
	sed -e '/^# DO NOT DELETE THIS LINE/,$$d' < Makefile.bak > Makefile
	echo '# DO NOT DELETE THIS LINE' >> Makefile
	echo ' ' >> Makefile
	rm Makefile.bak

depend:
	cp Makefile Makefile.bak
	sed -e '/^# DO NOT DELETE THIS LINE/,$$d' < Makefile.bak > Makefile
	echo '# DO NOT DELETE THIS LINE' >> Makefile
	echo ' ' >> Makefile
	for i in $(cfiles) ; do \
	  echo checking $$i ; \
	  $(CC) -M $(CPPFLAGS) $(srcdir)/$$i >> Makefile ; \
	done
# DO NOT DELETE THIS LINE
