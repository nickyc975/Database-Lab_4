vpath lib% lib
vpath % src

objects = libbptree.o libextmem.o libsort.o common.o main.o

main: $(objects)
	cc -o main $(objects)

libbptree.o: libbptree.h

libextmem.o: libextmem.h

libsort.o: libsort.h

common.o: common.h

main.o: common.h

clean:
	rm main $(objects) *.blk