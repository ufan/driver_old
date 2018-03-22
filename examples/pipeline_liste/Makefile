 CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat
#                -Wuninitialized -Werror
DRIVER_PATH  := ../..
CPPFLAGS     := -I$(DRIVER_PATH) -I$(DRIVER_PATH)/sis3100_calls
 

CFLAGS       := -g -ansi $(WFLAGS) -L$(DRIVER_PATH)/sis3100_calls

srcdir       := .
cfiles       := $(wildcard $(srcdir)/*.c)
EXEC         := $(cfiles:.c=)

.PHONY: all
all: $(EXEC)


pipeline: pipeline.c 
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100



clean:
	rm -f *.o core $(EXEC)

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
 




