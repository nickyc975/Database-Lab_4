vpath lib% lib
vpath % src

OUTPUT_DIR = out

CC = gcc
CFLAGS = -g -Wall

SRCS = libextmem.c stack.c bptree.c database.c main.c
OBJECTS = $(SRCS:.c=.o)

main: $(OBJECTS)
	$(if $(shell ls | grep -w $(OUTPUT_DIR)), , $(shell mkdir $(OUTPUT_DIR)))
	$(CC) $(CFLAGS) $(foreach obj, $(OBJECTS), $(OUTPUT_DIR)/$(obj)) -o $(OUTPUT_DIR)/main

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $(OUTPUT_DIR)/$@

run: clean main
	cd $(OUTPUT_DIR); ./main; cd ..

clean:
	rm -rf $(OUTPUT_DIR)/* *.o *.blk