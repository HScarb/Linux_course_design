# Makefile
all: init sender receiver
.PHONY: clean

init: init.o global.o
	gcc -pthread -o init init.o global.o

sender: sender.o global.o
	gcc -pthread -o sender sender.o global.o

receiver: receiver.o global.o
	gcc -pthread -o receiver receiver.o global.o

init.o: global.h

sender.o: global.h

receiver.o: global.h

clean:
	rm init
	rm receiver
	rm sender
	rm *.o
