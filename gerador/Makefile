CC = gcc
CFLAGS = -O2 -Wall -Wextra

all: gerador verificador

gerador: gerador.c
	$(CC) $(CFLAGS) -o $@ $<

verificador: verificador.c
	$(CC) $(CFLAGS) -o $@ $<

# Recria as instâncias geradas de entradas/ (exemplo_n5.txt é escrito à mão).
entradas: gerador
	set -e; for n in 20 100 1000 20000 100000; do ./gerador $$n 1 > entradas/n$${n}_s1.txt; done

clean:
	rm -f gerador verificador

.PHONY: all entradas clean
