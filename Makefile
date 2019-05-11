vpath lib% lib
vpath % src

OUTPUT_DIR = out

CC = gcc
CFLAGS = -g -Wall

SRCS = libextmem.c bptree.c common.c join.c project.c select.c main.c
OBJECTS = $(SRCS:.c=.o)

main: $(OBJECTS)
	$(if $(shell ls | grep -w $(OUTPUT_DIR)), , $(shell mkdir $(OUTPUT_DIR)))
	$(CC) $(CFLAGS) -o $(OUTPUT_DIR)/main $(OBJECTS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

test: clean main
	cd $(OUTPUT_DIR)
	$(OUTPUT_DIR)/main

clean:
	rm -rf $(OUTPUT_DIR)/* *.o *.blk