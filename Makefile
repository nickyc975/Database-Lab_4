vpath lib% lib
vpath % src

OUTPUT_DIR = out
OBJECTS = libextmem.o common.o bptree.o main.o

main: $(OBJECTS)
	$(if $(shell ls | grep -w $(OUTPUT_DIR)), , $(shell mkdir $(OUTPUT_DIR)))
	cc -o $(OUTPUT_DIR)/main $(OBJECTS)

libextmem.o: libextmem.h

common.o: common.h

bptree.o: bptree.h

main.o: common.h

clean:
	rm -rf $(OUTPUT_DIR)/* *.o