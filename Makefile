CC = gcc
CFLAGS = -c -g -Wall -fPIC
LDFLAGS = -shared -Xlinker -soname=libtask.so

HEADER = task.h

.PHONY:
	clean install uninstall

%.o: %.c
	$(CC) $^ $(CFLAGS)

libtask.so: *.o
	$(CC) $^ -o libtask.so $(LDFLAGS)

clean:
	rm *.o

install:
	sudo cp $(HEADER) /usr/include
	mv libtask.so /usr/lib
	chmod 0755 /usr/lib/libtask.so	
	ldconfig -n libtask.so

uninstall:
	rm /usr/lib/libtask.so
	rm /usr/include/$(HEADER)
	ldconfig
