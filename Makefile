all:
	gcc -g test.c fs.c ext.c -o sfs
witha1:
	gcc -g a1/caching.c a1/mydisk.c fs.c test.c exta1.c -o sfs
clean:
	rm -rf *.out *.o a1/*.o sfs storage
