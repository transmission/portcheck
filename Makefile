CFLAGS = -g -O3 -Wall
LDFLAGS = -levent

.PHONY: test

portcheck: portcheck.o
	$(CC) -o $@ $< $(LDFLAGS)

test:
	python3 -m unittest
