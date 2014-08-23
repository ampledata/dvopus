all: dvopus-tx dvopus-rx 

dvopus-tx: dvopus-tx.c
	gcc -o dvopus-tx dvopus-tx.c -lpulse-simple -lopus -lz
dvopus-rx: dvopus-rx.c
	gcc -o dvopus-rx dvopus-rx.c -lpulse-simple -lopus -lz

