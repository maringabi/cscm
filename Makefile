CC = cc
CFLAGS = -Wall -g

cscm: cscm.c
	$(CC) $(CFLAGS) $< -o $@
