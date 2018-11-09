CC = clang
CFLAGS = -Wall -g
LD = clang
LDFLAGS =

all: TARheader

TARheader: TARheader.o
	$(LD) $(LDFLAGS) -o TARheader TARheader.o

TARheader.o: TARheader.c
	$(CC) $(CFLAGS) -o TARheader.o -c TARheader.c

clean:
	rm TARheader *.o
