CC=/usr/bin/cc

all: cachesim pipeline disasm

cachesim: 
	$(CC) cachesim.c -o cachesim

pipeline:
	$(CC) pipeline.c -o pipeline

disasm:
	$(CC) disasm.c -o disasm

clean:
	-rm cachesim pipeline disasm
