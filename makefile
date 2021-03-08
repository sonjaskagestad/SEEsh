all: SEEsh.exe

SEEsh.exe: SEEsh.o
	 gcc -o SEEsh.exe SEEsh.o

SEEsh.o: SEEsh.c
	 gcc -c -Werror -Wall SEEsh.c

clean:
	 rm SEEsh.o SEEsh.exe
