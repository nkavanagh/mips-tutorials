/**
 * Niall Kavanagh <niall@kst.com>
 * MIPS Cache simulation
 * Compiled and run on OS X
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cachesim.h"

/* Cache block */
struct _Cache_Slot {
	short valid;
	short dirty;
	unsigned char tag;
	unsigned char data[CACHE_BLOCK_SIZE];
};

/* main */
int main(int argc, char *argv[]) {
	slots = NULL;
	char input[INPUT_BUFFER_SIZE];
	char *uargv[INPUT_ARGS]; 
	int uargc;
	
	initialize_memory();
	initialize_cache();
	
	/* input loop */
	printf("Enter '?' for help.\n");
	
	while(1){
	        printf("> ");
	        fgets(input, INPUT_BUFFER_SIZE, stdin);
	
			uargc = parse_command(input, uargv);

			if (uargc == 0) {
				/* No arguments? Loop again */
				continue;
			}
			
			if ((strcmp(uargv[0], "exit") == 0) || (strcmp(uargv[0], "q") == 0)) {
				/* normal exit */
				return 0; 
			} else if (strcmp(uargv[0], "?") == 0) {
				print_help();
			} else if (strcmp(uargv[0], "pm") == 0) {
				print_memory();
			} else if (strcmp(uargv[0], "pc") == 0) {
				print_cache();
			} else if (strcmp(uargv[0], "im") == 0) {
				initialize_memory();
			} else if (strcmp(uargv[0], "ic") == 0) {
				initialize_cache();
			} else if (strcmp(uargv[0], "r") == 0) {
				if (uargc == 2) {
					/* strtol *could* overflow our short address */
					short address = strtol(uargv[1], NULL, 16); 
					
					int is_cache_hit = 0;
					unsigned char byte = read_byte(address, &is_cache_hit);
					printf("Address\tData\tHit/Miss\n0x%X\t%X\t%s\n", address, byte, (is_cache_hit) ? "HIT" : "MISS");
				} else {
					printf("Invalid command. To read a byte: r <address>; e.g. 'r 7ae'\n");
				}
			} else if (strcmp(uargv[0], "w") == 0) {
				if (uargc == 3) {
					/* strtol *could* overflow */
					short address = strtol(uargv[1], NULL, 16); 
					unsigned char byte = strtol(uargv[2], NULL, 16);
					int is_cache_hit = write_byte(address, byte);
					printf("Address\tData\tHit/Miss\n0x%X\t%X\t%s\n", address, byte, (is_cache_hit) ? "HIT" : "MISS");
				} else {
					printf("Invalid command. To write a byte: w <address> <value>; e.g. 'w 7ae 2b'\n");
				} 
			} else {
				printf("Unknown or incorrect command. Enter '?' for help.\n");
			}
			
			printf("\n");
	}
	
    return 0;
}

int parse_command(const char *cmdline, char *arglist[]) {
	static char array[INPUT_BUFFER_SIZE]; /* holds local copy of command line */
	char *buf = array;          /* ptr that traverses command line */
	char *delim;                /* points to first space delimiter */
	int argc;                   /* number of args */

	strncpy(buf, cmdline, strlen(cmdline)+1);
	buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* ignore leading spaces */
		buf++;

	/* Build the arglist list */
	argc = 0;
	/* based on delimiter " "(space), separate the commandline into arglist */
	while ((delim = strchr(buf, ' ')) && (argc < INPUT_ARGS - 1)) {
		arglist[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* ignore spaces */
			buf++;
	}
	arglist[argc] = NULL;
	return argc;
}

/** 
 * "Zero" out main memory using 0x00–0xFF
 */
void initialize_memory() {
	unsigned char current_value = 0;
	
	for (int n=0; n < MEMORY_SIZE; n++) {
		main_memory[n] = current_value;
		current_value++;
	}
}

/**
 * Dump the contents of main memory
 */
void print_memory() {
	printf("Address\t\tContents\n");
	for (int n=0; n < MEMORY_SIZE; n++) {
		printf("0x%X\t\t0x%X\n", n, main_memory[n]);
	}
}

/**
 * Initialize the cache slots
 */
void initialize_cache() {
	if (slots != NULL) {
		free(slots);
		slots = NULL;
	}
	
	slots = malloc(sizeof(Cache_Slot*) * CACHE_SLOTS);
	
	for (int n=0; n < CACHE_SLOTS; n++) {
		slots[n].valid = 0;
		slots[n].dirty = 0;
		slots[n].tag = 0;
		
		for (int i=0; i < CACHE_BLOCK_SIZE; i++) {
			slots[n].data[i] = 0x0;
		}
	}
}

/**
 * Dump out the current contents of the cache
 */
void print_cache() {
	if (slots != NULL) {
		printf("Slot\tValid\tDirty\tTag\tData\n");
		
		for (int n=0; n < CACHE_SLOTS; n++) {
			printf("%x\t%d\t%d\t%2X\t",
				n, 
				slots[n].valid, 
				slots[n].dirty,
				slots[n].tag);
				
			for (int i=0; i < CACHE_BLOCK_SIZE; i++) {
				printf("%2X ", slots[n].data[i]);
			}
			
			printf("\n");
		}
	} else {
		printf("[!] Cache is not initialized.\n");
	}
}

/**
 * Extracting cache slot fields from addresses:
 * 16 byte block size
 * 16 blocks of 16 "addresses" is 2048 bits
 * Single address as short is 16 bits
 * Single cache slot holds 16 addresses for 128 bits
 * Direct mapped, 16 sets
 * Displacement is 4
 * Block bits is 4
 * 8 bit tag
 */
unsigned char address_tag(short address) {
	unsigned char tag = address >> 8;
	return tag;
}

short address_index(short address) {
	short index = (address >> 4) & 0x000F;
	return index;
}

short address_offset(short address) {
	short offset = address & 0x000F;
	return offset;
}

short address_block_base(short address) {
	short base_addr = address & 0x0FF0;
	return base_addr;
}

/**
 * Read a byte of data from an address
 */
unsigned char read_byte(short address, int *is_cache_hit) {
	*is_cache_hit = 0;
	unsigned char byte = 0;
	
	unsigned char tag = address_tag(address);
	short index = address_index(address);
	short offset = address_offset(address);
	short base_addr = address_block_base(address);
	
	if ((slots[index].tag == tag) && (slots[index].valid == 1)) {
		/* block is in the cache and valid */
		*is_cache_hit = 1;
	} else {
		/* fetch the block from main memory */
		fetch_block(address);	
	}
	
	byte = slots[index].data[offset];
	
	return byte;
}

/**
 * Write a byte of data to an address
 */
int write_byte(short address, unsigned char byte) {
	int is_cache_hit = 0;
	
	unsigned char tag = address_tag(address);
	short index = address_index(address);
	short offset = address_offset(address);
	short base_addr = address_block_base(address);
	
	/* first, check the cache */
	if ((slots[index].tag == tag) && (slots[index].valid == 1)) {
		/* block is in the cache and valid */
		is_cache_hit = 1;
	} else {
		/* fetch block and put it in the cache */
		fetch_block(address);	
	}
	
	/* set the byte */
	slots[index].data[offset] = byte;
	slots[index].dirty = 1;
	
	return is_cache_hit;
}

/**
 * Fetch a block of data from main memory and place it in the cache
 * If a dirty block occupies the slot, flush it
 */
void fetch_block(short address) {
	unsigned char tag = address_tag(address);
	short index = address_index(address);
	short offset = address_offset(address);
	short base_addr = address_block_base(address);
	
	/* If the cache slot is dirty, flush it */
	if (slots[index].dirty == 1) {
		flush_slot(index);
	}
	
	/* Reset the slot */
	slots[index].dirty = 0;
	slots[index].tag = tag;
	
	short current = base_addr;
	for (int i=0; i < CACHE_BLOCK_SIZE; i++) {
		slots[index].data[i] = main_memory[current];
		current += sizeof(unsigned char);
	}
	
	slots[index].valid = 1;
}

/**
 * Flush a (presumably dirty) cache slot out to main memory
 */
void flush_slot(short index) {
	short base_addr = ((slots[index].tag << 8) & 0x0F00) | ((index << 4) & 0x00F0);
	
	slots[index].valid = 0;
	
	short current = base_addr;
	for (int i=0; i < CACHE_BLOCK_SIZE; i++) {
		main_memory[current] = slots[index].data[i];
		slots[index].data[i] = 0;
		current += sizeof(unsigned char);
	}
	
	slots[index].dirty = 0;
	slots[index].tag = 0;
}

/**
 * Print out a list of commands
 */
void print_help() {
	printf("Command\tDescription\t\t\t\tExample\n\n");
	
	printf("r\tRead byte from address\t\t\tr 7ae\n");
	printf("w\tWrite byte to address\t\t\tw 7ae 2b\n");
	printf("pc\tPrint cache contents\t\t\tpc\n");
	printf("pm\tPrint memory contents\t\t\tpm\n");
	printf("q\tQuit cache simulation\t\t\tq\n");
}
