# Specify compiler
CC=gcc
 
# Specify linker
LINK=gcc

OUTFILE=client

output: 
	$(CC) src/main.c src/network.c src/network.h src/file_man.c src/file_man.h -o $(OUTFILE) -lws2_32

clean:
	-del $(OUTFILE).exe