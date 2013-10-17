##
## usbmon - simple front-end for in-kernel usbmon
##

#CFLAGS += -Wall -Wp,-D_FORTIFY_SOURCE=2 -O2
CFLAGS += -Wall -Wshadow -Wmissing-declarations -Wmissing-prototypes
CFLAGS += -Wnested-externs -Wpointer-arith -Wpointer-arith -Wsign-compare
CFLAGS += -Wchar-subscripts -Wstrict-prototypes -Wformat=2 -Wtype-limits
CFLAGS += -Wp,-D_FORTIFY_SOURCE=2
CFLAGS += -O2

all: usbmon

usbmon: usbmon.o

usbmon.o: usbmon.c

clean:
	rm -f *.o usbmon
