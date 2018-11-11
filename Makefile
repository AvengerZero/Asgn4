CC = clang
CFLAGS = -Wall -g
LD = clang
LDFLAGS =

all: mytar

mytar: mytar.o TARheader.o
	$(LD) $(LDFLAGS) -o mytar mytar.o TARheader.o

TARheader.o: TARheader.c
	$(CC) $(CFLAGS) -o TARheader.o -c TARheader.c

mytar.o: mytar.c
	$(CC) $(CFLAGS) -o mytar.o -c mytar.c

clean:
	rm mytar *.o
