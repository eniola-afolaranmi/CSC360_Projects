.phony all:
all: mts

mts: mts.c
	gcc mts.c -lpthread -lhistory -Wall -o mts
	

.PHONY clean:
clean:
	-rm -rf *.o *.exe
