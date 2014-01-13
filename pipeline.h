/**
 * Niall Kavanagh <niall@kst.com>
 * MIPS Pipeline simulation
 */

#ifndef Pipeline_main_h
#define Pipeline_main_h


#define MEMORY_SIZE 1024 // 1K
#define NUM_REGISTERS 32

#define NOOP 0x00000000

#define X -1

#define PR_WRITE 0
#define PR_READ 1

typedef struct _IF_ID_Reg IF_ID_Reg;
IF_ID_Reg *IF_ID;

typedef struct _ID_EX_Reg ID_EX_Reg;
ID_EX_Reg *ID_EX;

typedef struct _EX_MEM_Reg EX_MEM_Reg;
EX_MEM_Reg *EX_MEM;

typedef struct _MEM_WB_Reg MEM_WB_Reg;
MEM_WB_Reg *MEM_WB;

short main_memory[MEMORY_SIZE];
short registers[NUM_REGISTERS];

uint32_t instructions[] = {
	0xa1020000, // sb $2,0($8)
	0x810AFFFC, // lb $10,-4($8)
	0x00831820, // add $3,$4,$3
	0x01263820, // add $7,$9,$6
	0x01224820, // add $9,$9,$2
	0x81180000, // lb $24,0($8)
	0x81510010, // lb $17,16($10)
	0x00624022, // sub $8,$3,$2
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
};

short ctr;

void initialize_memory();
void initialize_registers();

void instr_fetch();
void instr_decode();
void execute();
void memory_access();
void write_back();
void copy_to_read();
void print_registers();

void desc_instr(uint32_t instr, char *desc);
unsigned int get_opcode(uint32_t instr);
unsigned int get_rs(uint32_t instr);
unsigned int get_rt(uint32_t instr);
unsigned int get_rd(uint32_t instr);
unsigned int get_shamt(uint32_t instr);
unsigned int get_funct(uint32_t instr);
unsigned int get_immediate(uint32_t instr);


#endif
