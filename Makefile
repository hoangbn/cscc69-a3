COMPILE = gcc -Wall -g -o $@ $^

all: ext2_ls ext2_cp ext2_mkdir ext2_ln ext2_rm ext2_rm_bonus
	
ext2_ls: ext2_ls.o diskload.o utils.o
	$(COMPILE)

ext2_cp: ext2_cp.o diskload.o utils.o
	$(COMPILE)

ext2_mkdir: ext2_mkdir.o diskload.o utils.o
	$(COMPILE)

ext2_ln: ext2_ln.o diskload.o utils.o
	$(COMPILE)

ext2_rm: ext2_rm.o diskload.o utils.o
	$(COMPILE)

ext2_rm_bonus: ext2_rm_bonus.o diskload.o utils.o
	$(COMPILE)

%.o: %.c diskload.h utils.h
	gcc -Wall -g -c $<

clean :
	rm -f *.o
