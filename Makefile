CFLAGS=-Wall
Pthread = -lpthread
Math = -lm
RealTime = -lrt

all: dph cons prod mycall

dph:
	gcc $(CFLAGS) -o dph dph.c $(Pthread) $(Math) $(RealTime)

cons:
	gcc $(CFLAGS) -o cons cons.c $(Pthread) $(Math) $(RealTime)

prod:
	gcc $(CFLAGS) -o prod prod.c $(Pthread) $(Math) $(RealTime)

mycall:
	gcc $(CFLAGS) -o mycall mycall.c $(Pthread) $(Math) $(RealTime)

clean:
	rm -rf test
	rm -rf dph
	rm -rf prod
	rm -rf cons
	rm -rf mycall
