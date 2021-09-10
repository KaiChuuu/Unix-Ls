all:
	gcc -c UnixLs.c
	gcc -o UnixLs UnixLs.o
clean:
	rm UnixLs
	rm UnixLs.o