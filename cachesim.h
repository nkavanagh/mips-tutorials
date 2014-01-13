/**
 * Niall Kavanagh <niall@kst.com>
 * MIPS Cache Simulator
 */

/* 2K main memory */
#define MEMORY_SIZE 2048

#define CACHE_BLOCK_SIZE 16
#define CACHE_SLOTS 16

/* user input */
#define INPUT_BUFFER_SIZE 1024
#define INPUT_ARGS 4

/* Array to hold byte-addressable "main memory" */
short main_memory[MEMORY_SIZE];

/* Cache slot struct and array for slots */
typedef struct _Cache_Slot Cache_Slot;
Cache_Slot *slots;

unsigned char read_byte(short address, int *is_cache_hit);
int write_byte(short address, unsigned char byte);

void fetch_block(short address);
void flush_slot(short index);

void initialize_memory();
void initialize_cache();

void print_memory();
void print_cache();
void print_help();

int parse_command(const char *cmdline, char *arglist[]);