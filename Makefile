CC=gcc
CFLAGS=-Wall -g

TARGETS=lc3as decas

all: lc3as decas

lc3as: LC3-Assembler.c
	$(CC) $(CFLAGS) $< -o $@

decas: Decimal-Assembler.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

