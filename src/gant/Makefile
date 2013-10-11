CFLAGS=-g -Wall `xml2-config --cflags`
LDLIBS=-lpthread -lm `xml2-config --libs`

all:	gant

gant: gant.o antlib.o

gant.o:	gant.c antdefs.h

antlib.o: antlib.c antdefs.h

clean:
	-rm *.o gant
