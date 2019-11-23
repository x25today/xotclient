CFLAGS=-Wall -ggdb

$(.c:.o):	
	gcc -o $@ $(CFLAGS) $< 
xotclient: xotclient.o tcp.o x25.o tty.o
	gcc -o xotclient xotclient.o tcp.o x25.o tty.o -lncurses
	
	
clean:
	rm -f xotclient *.o

