COMPILE = gcc -Wall -g -o $@ $^

all: ext2_ls ext2_mkdir
	
ext2_ls: ext2_ls.o diskload.o utils.o
	$(COMPILE)

ext2_mkdir: ext2_mkdir.o diskload.o utils.o
	$(COMPILE)

%.o: %.c diskload.h utils.h
	gcc -Wall -g -c $<

clean :
	rm -f *.o
