CFLAGS=-g -Wall
LDFLAGS=-lpthread -lm

all:	gant

gant: gant.o antlib.o

gant.o:	gant.c antdefs.h

antlib.o: antlib.c antdefs.h

clean:
	rm *.o gant
