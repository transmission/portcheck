CFLAGS = -g -O3 -Wall
LDFLAGS = -levent

portcheck: portcheck.o
	$(CC) -o $@ $< $(LDFLAGS)
