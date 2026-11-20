# xonraOS for Raspberry Pi. Needs elk.c and elk.h (Elk is AGPL/commercial, so they are not copied here):
#   git clone https://github.com/cesanta/elk   ->   make ELK=elk
ELK ?= elk
CFLAGS ?= -O2 -Wall -I$(ELK) -I.

xonra: xonra.c font5x7.h
	$(CC) $(CFLAGS) -o $@ xonra.c $(ELK)/elk.c -lm -lpthread

# Text-screen simulator for any computer (no Pi needed). Try:  printf 'tp' | XONRA_HOME=./simhome ./xonra-sim
sim: xonra.c font5x7.h
	$(CC) $(CFLAGS) -DXONRA_SIM -o xonra-sim xonra.c $(ELK)/elk.c -lm

clean:
	rm -f xonra xonra-sim
