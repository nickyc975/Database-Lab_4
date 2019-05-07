vpath lib% lib
vpath % src

OUTPUT_DIR = out
OBJECTS = libbptree.o libextmem.o libsort.o common.o main.o

main: $(OBJECTS)
	$(if $(shell ls | grep -w $(OUTPUT_DIR)), , $(shell mkdir $(OUTPUT_DIR)))
	cc -o $(OUTPUT_DIR)/main $(OBJECTS)

libbptree.o: libbptree.h

libextmem.o: libextmem.h

libsort.o: libsort.h

common.o: common.h

main.o: common.h

clean:
	rm -rf $(OUTPUT_DIR)/* *.o