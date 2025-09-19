CC = cc
CFLAGS = -W -Wall -O2

cscm: cscm.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf a.out cscm cscm.dSYM
