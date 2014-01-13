/**
 * Niall Kavanagh <niall@kst.com>
 * MIPS Pipeline simulation
 * Compiled and run on OS X
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pipeline.h"

struct _IF_ID_Reg {
	uint32_t instr;
};

struct _ID_EX_Reg {
	uint32_t instr;
    short RegDst;
    short ALUSrc;
    short ALUOp;
    short MemRead;
    short MemWrite;
    short MemToReg;
    short RegWrite;
    short ReadReg1Value;
    short ReadReg2Value;
    short SEOffset;
    short WriteReg1Num;
    short WriteReg2Num;
};

struct _EX_MEM_Reg {
    uint32_t instr;
    short MemRead;
    short MemWrite;
    short MemToReg;
    short RegWrite;
    short ALUResult;
    short SWValue;
    short WriteRegNum;
};

struct _MEM_WB_Reg {
    uint32_t instr;
    short MemRead;
    short MemWrite;
    short MemToReg;
    short RegWrite;
    short ALUResult;
    short SWValue;
    short LWDataValue;
    short WriteRegNum;
};

/* main */
int main(int argc, char *argv[]) {
	ctr = 0;
	IF_ID = NULL;
	ID_EX = NULL;
	EX_MEM = NULL;
    MEM_WB = NULL;
	
	size_t num_instructions = sizeof(instructions)/sizeof(uint32_t);
	
	initialize_memory();
	initialize_registers();
	
    printf("Disassembling %ld instructions and running them ", num_instructions);
    printf("through our pipeline simulation.\n");
    printf("-1 is used as a \"don't care\" value (e.g. 0xFFFFFFFF, -1, etc.)\n\n");
    
	while (ctr < num_instructions) {
		instr_fetch();
		instr_decode();
		execute();
		memory_access();
		write_back();
        
		print_registers();
		
		copy_to_read();
	}
	
	return 0;
}

/**
 * IF - Instruction Fetch
 * Fetch the next instruction out of the Instruction Cache.
 * Put it in the WRITE version of the IF/ID pipeline register.
 */
void instr_fetch() {
	uint32_t instr = instructions[ctr];
	ctr++;
    
	IF_ID[PR_WRITE].instr = instr;
}

/**
 * ID - Instruction Decode
 * nop, add, sub, sb and lb
 * read an instruction from the READ version of IF/ID pipeline register,
 * do the decoding and register fetching and write the values to the
 * WRITE version of the ID/EX pipeline register.
 */
void instr_decode() {
	uint32_t instr = IF_ID[PR_READ].instr;
	
    ID_EX[PR_WRITE].instr = instr;
    
	/* decode and fetch */
	if (instr != NOOP) {
		switch (get_opcode(instr)) {
			case 0x0:
                /* add or sub */
                ID_EX[PR_WRITE].RegDst = 1;
                ID_EX[PR_WRITE].ALUSrc = 0;
                ID_EX[PR_WRITE].ALUOp = 2; // 0b10
                ID_EX[PR_WRITE].MemRead = 0;
                ID_EX[PR_WRITE].MemWrite = 0;
                ID_EX[PR_WRITE].MemToReg = 0;
                ID_EX[PR_WRITE].RegWrite = 1;
                
                ID_EX[PR_WRITE].ReadReg1Value = registers[get_rs(instr)];
                ID_EX[PR_WRITE].ReadReg2Value = registers[get_rt(instr)];
                ID_EX[PR_WRITE].SEOffset = X;
                ID_EX[PR_WRITE].WriteReg1Num = get_rt(instr);
                ID_EX[PR_WRITE].WriteReg2Num = get_rd(instr);
                break;
                
			case 0x20: /* lb */
                ID_EX[PR_WRITE].RegDst = 0;
                ID_EX[PR_WRITE].ALUSrc = 1;
                ID_EX[PR_WRITE].ALUOp = 0;
                ID_EX[PR_WRITE].MemRead = 1;
                ID_EX[PR_WRITE].MemWrite = 0;
                ID_EX[PR_WRITE].MemToReg = 1;
                ID_EX[PR_WRITE].RegWrite = 1;
                
                ID_EX[PR_WRITE].ReadReg1Value = registers[get_rs(instr)];
                ID_EX[PR_WRITE].ReadReg2Value = registers[get_rt(instr)];
                ID_EX[PR_WRITE].SEOffset = get_immediate(instr);
                ID_EX[PR_WRITE].WriteReg1Num = get_rt(instr);
                ID_EX[PR_WRITE].WriteReg2Num = get_rs(instr);
                break;
                
			case 0x28: /* sb */
                ID_EX[PR_WRITE].RegDst = X;
                ID_EX[PR_WRITE].ALUSrc = 1;
                ID_EX[PR_WRITE].ALUOp = 0;
                ID_EX[PR_WRITE].MemRead = 0;
                ID_EX[PR_WRITE].MemWrite = 1;
                ID_EX[PR_WRITE].MemToReg = X;
                ID_EX[PR_WRITE].RegWrite = 0;
                
                ID_EX[PR_WRITE].ReadReg1Value = registers[get_rs(instr)];
                ID_EX[PR_WRITE].ReadReg2Value = registers[get_rt(instr)];
                ID_EX[PR_WRITE].SEOffset = get_immediate(instr);
                ID_EX[PR_WRITE].WriteReg1Num = get_rt(instr);
                ID_EX[PR_WRITE].WriteReg2Num = get_rs(instr);
                break;
		}
	}
}

/**
 * EX - Execute
 * Perform the requested instruction on the specific operands read out of
 * the READ version of the IDEX pipeline register and then write the
 * appropriate values to the WRITE version of the EX/MEM pipeline register.
 */
void execute() {
	uint32_t instr = ID_EX[PR_READ].instr;
	
    EX_MEM[PR_WRITE].instr = instr;
    EX_MEM[PR_WRITE].MemRead = ID_EX[PR_READ].MemRead;
    EX_MEM[PR_WRITE].MemWrite = ID_EX[PR_READ].MemWrite;
    EX_MEM[PR_WRITE].MemToReg = ID_EX[PR_READ].MemToReg;
    EX_MEM[PR_WRITE].RegWrite = ID_EX[PR_READ].RegWrite;
    
    if (ID_EX[PR_READ].RegDst == 0) {
        EX_MEM[PR_WRITE].WriteRegNum = ID_EX[PR_READ].WriteReg1Num;
    } else if (ID_EX[PR_READ].RegDst == 1) {
        EX_MEM[PR_WRITE].WriteRegNum = ID_EX[PR_READ].WriteReg2Num;
    } else {
        EX_MEM[PR_WRITE].WriteRegNum = X;
    }
    
    if (instr != NOOP) {
		switch (get_opcode(instr)) {
			case 0x0:
                switch (get_funct(instr)) {
                    case 0x20: /* add */
                        EX_MEM[PR_WRITE].ALUResult = ID_EX[PR_READ].ReadReg1Value + ID_EX[PR_READ].ReadReg2Value;
                        EX_MEM[PR_WRITE].SWValue = ID_EX[PR_READ].ReadReg2Value;
                        break;
                        
                        
                    case 0x22: /* sub */
                        EX_MEM[PR_WRITE].ALUResult = ID_EX[PR_READ].ReadReg1Value - ID_EX[PR_READ].ReadReg2Value;
                        EX_MEM[PR_WRITE].SWValue = ID_EX[PR_READ].ReadReg2Value;
                        break;
                }
                
                break;
                
			case 0x20: /* lb */
                EX_MEM[PR_WRITE].ALUResult = ID_EX[PR_READ].ReadReg1Value + ID_EX[PR_READ].SEOffset;
                EX_MEM[PR_WRITE].SWValue = ID_EX[PR_READ].ReadReg2Value;
                break;
                
			case 0x28: /* sb */
                EX_MEM[PR_WRITE].ALUResult = ID_EX[PR_READ].ReadReg1Value + ID_EX[PR_READ].SEOffset;
                EX_MEM[PR_WRITE].SWValue = ID_EX[PR_READ].ReadReg2Value;
                break;
		}
	}
}

/**
 * MEM - Memory Access
 * If the instruction is a lb, then use the address you calculated in the
 * EX stage as an index into your Main Memory array and get the value that
 * is there.  Otherwise, just pass information from the READ version of the
 * EX_MEM pipeline register to the WRITE version of MEM_WB.
 */
void memory_access() {
    uint32_t instr = EX_MEM[PR_READ].instr;
	
    MEM_WB[PR_WRITE].instr = instr;
    MEM_WB[PR_WRITE].MemRead = EX_MEM[PR_READ].MemRead;
    MEM_WB[PR_WRITE].MemWrite = EX_MEM[PR_READ].MemWrite;
    MEM_WB[PR_WRITE].MemToReg = EX_MEM[PR_READ].MemToReg;
    MEM_WB[PR_WRITE].RegWrite = EX_MEM[PR_READ].RegWrite;
    MEM_WB[PR_WRITE].ALUResult = EX_MEM[PR_READ].ALUResult;
    MEM_WB[PR_WRITE].SWValue = EX_MEM[PR_READ].SWValue;
    MEM_WB[PR_WRITE].WriteRegNum = EX_MEM[PR_READ].WriteRegNum;
    
    if (MEM_WB[PR_WRITE].MemRead == 1) {
        MEM_WB[PR_WRITE].LWDataValue = main_memory[MEM_WB[PR_WRITE].ALUResult];
    } else if (MEM_WB[PR_WRITE].MemWrite == 1) {
        main_memory[MEM_WB[PR_WRITE].ALUResult] = MEM_WB[PR_WRITE].SWValue & 0xff;
    } else {
        MEM_WB[PR_WRITE].LWDataValue = X;
    }
}

/**
 * WB - Register Write Back
 * Write to the registers based on information you read out of the
 * READ version of MEM_WB
 */
void write_back() {
	if (MEM_WB[PR_READ].RegWrite == 1) {
        if (MEM_WB[PR_READ].MemToReg == 1) {
            /* lb */
            registers[MEM_WB[PR_READ].WriteRegNum] = MEM_WB[PR_READ].LWDataValue;
        } else if (MEM_WB[PR_READ].MemToReg == 0){
            /* add, sub */
            registers[MEM_WB[PR_READ].WriteRegNum] = MEM_WB[PR_READ].ALUResult;
        }
    }
}

/**
 *
 */
void copy_to_read() {
    IF_ID[PR_READ] = IF_ID[PR_WRITE];
    ID_EX[PR_READ] = ID_EX[PR_WRITE];
    EX_MEM[PR_READ] = EX_MEM[PR_WRITE];
    MEM_WB[PR_READ] = MEM_WB[PR_WRITE];
}

/**
 * Print out the contents of our registers
 */
void print_registers() {
	printf("==============================================================\n");
	printf("Clock Cycle #%d\n", ctr);
	printf("==============================================================\n");
    
    int n = 0;
    
    while (n < NUM_REGISTERS) {
        printf("%02d: 0x%08x\t%02d: 0x%08x\t%02d: 0x%08x\t%02d: 0x%08x\n",
               n, registers[n++],
               n, registers[n++],
               n, registers[n++],
               n, registers[n++]
               );
    }
	
	char desc[18];
	
    /* IF/ID */
	desc_instr(IF_ID[PR_WRITE].instr, desc);
	printf("\n%14s\t0x%08x\t%s\n\n",
           "IF/ID Write:",
           IF_ID[PR_WRITE].instr,
           desc);
    
	desc_instr(IF_ID[PR_READ].instr, desc);
	printf("%14s\t0x%08x\t%s\n\n",
           "IF/ID Read:",
           IF_ID[PR_READ].instr, desc);
	
    /* ID/EX */
	desc_instr(ID_EX[PR_WRITE].instr, desc);
	printf("%14s\t0x%08x\t%s\n",
           "ID/EX Write:", ID_EX[PR_WRITE].instr,
           desc);
    
    if (ID_EX[PR_WRITE].instr != NOOP) {
        printf("%9s: %d\t%9s: %d\t%9s: %d\t%9s: %d\n",
               "RegDst", ID_EX[PR_WRITE].RegDst,
               "ALUSrc", ID_EX[PR_WRITE].ALUSrc,
               "ALUOp", ID_EX[PR_WRITE].ALUOp,
               "MemRead", ID_EX[PR_WRITE].MemRead);
        printf("%9s: %d\t%9s: %d\t%9s: %d\n",
               "MemWrite", ID_EX[PR_WRITE].MemWrite,
               "MemToReg", ID_EX[PR_WRITE].MemToReg,
               "RegWrite", ID_EX[PR_WRITE].RegWrite);
        printf("%13s: 0x%08x\t%13s: 0x%08x\n",
               "ReadReg1Value", ID_EX[PR_WRITE].ReadReg1Value,
               "ReadReg2Value", ID_EX[PR_WRITE].ReadReg2Value);
        printf("%13s: 0x%08x\t%13s: %d,%d\n",
               "SEOffset", ID_EX[PR_WRITE].SEOffset,
               "WriteRegNum", ID_EX[PR_WRITE].WriteReg1Num, ID_EX[PR_WRITE].WriteReg2Num);
    }
    
	desc_instr(ID_EX[PR_READ].instr, desc);
	printf("\n%14s\t0x%08x\t%s\n",
           "ID/EX Read:",
           ID_EX[PR_READ].instr, desc);
    
    if (ID_EX[PR_READ].instr != NOOP) {
        printf("%9s: %d\t%9s: %d\t%9s: %d\t%9s: %d\n",
               "RegDst", ID_EX[PR_READ].RegDst,
               "ALUSrc", ID_EX[PR_READ].ALUSrc,
               "ALUOp", ID_EX[PR_READ].ALUOp,
               "MemRead", ID_EX[PR_READ].MemRead);
        printf("%9s: %d\t%9s: %d\t%9s: %d\n",
               "MemWrite", ID_EX[PR_READ].MemWrite,
               "MemToReg", ID_EX[PR_READ].MemToReg,
               "RegWrite", ID_EX[PR_READ].RegWrite);
        printf("%13s: 0x%08x\t%13s: 0x%08x\n",
               "ReadReg1Value", ID_EX[PR_READ].ReadReg1Value,
               "ReadReg2Value", ID_EX[PR_READ].ReadReg2Value);
        printf("%13s: 0x%08x\t%13s: %d,%d\n",
               "SEOffset", ID_EX[PR_READ].SEOffset,
               "WriteRegNum", ID_EX[PR_READ].WriteReg1Num, ID_EX[PR_READ].WriteReg2Num);
    }
    
    /* EX/MEM */
	desc_instr(EX_MEM[PR_WRITE].instr, desc);
	printf("\n%14s\t0x%08x\t%s\n",
           "EX/MEM Write:", EX_MEM[PR_WRITE].instr, desc);
    
    if (EX_MEM[PR_WRITE].instr != NOOP) {
        printf("%9s: %d\t%9s: %d\t%9s: %d\t%9s: %d\n",
               "MemRead", EX_MEM[PR_WRITE].MemRead,
               "MemWrite", EX_MEM[PR_WRITE].MemWrite,
               "MemToReg", EX_MEM[PR_WRITE].MemToReg,
               "RegWrite", EX_MEM[PR_WRITE].RegWrite);
        printf("%11s: 0x%08x\t%11s: 0x%08x\t%11s: %d\n",
               "ALUResult", EX_MEM[PR_WRITE].ALUResult,
               "SWValue", EX_MEM[PR_WRITE].SWValue,
               "WriteRegNum", EX_MEM[PR_WRITE].WriteRegNum);
    }
    
	desc_instr(EX_MEM[PR_READ].instr, desc);
	printf("\n%14s\t0x%08x\t%s\n",
           "EX/MEM  Read:", EX_MEM[PR_READ].instr, desc);
    
    if (EX_MEM[PR_READ].instr != NOOP) {
        printf("%9s: %d\t%9s: %d\t%9s: %d\t%9s: %d\n",
               "MemRead", EX_MEM[PR_READ].MemRead,
               "MemWrite", EX_MEM[PR_READ].MemWrite,
               "MemToReg", EX_MEM[PR_READ].MemToReg,
               "RegWrite", EX_MEM[PR_READ].RegWrite);
        printf("%11s: 0x%08x\t%11s: 0x%08x\t%11s: %d\n",
               "ALUResult", EX_MEM[PR_READ].ALUResult,
               "SWValue", EX_MEM[PR_READ].SWValue,
               "WriteRegNum", EX_MEM[PR_READ].WriteRegNum);
    }
    
    /* MEM/WB */
	desc_instr(MEM_WB[PR_WRITE].instr, desc);
    printf("\nMEM/WB Write:\t0x%08x\t%s\n", MEM_WB[PR_WRITE].instr, desc);
    
    if (MEM_WB[PR_WRITE].instr != NOOP) {
        printf("%8s: %d\t%8s: %d\n",
               "MemToReg", MEM_WB[PR_WRITE].MemToReg,
               "RegWrite", MEM_WB[PR_WRITE].RegWrite);
        printf("%11s: 0x%08x\t%11s: 0x%08x\t%11s: %d\n",
               "LWDataValue", MEM_WB[PR_WRITE].LWDataValue,
               "ALUResult", MEM_WB[PR_WRITE].ALUResult,
               "WriteRegNum", MEM_WB[PR_WRITE].WriteRegNum);
    }

    desc_instr(MEM_WB[PR_READ].instr, desc);
    printf("\nMEM/WB Read:\t0x%08x\t%s\n", MEM_WB[PR_READ].instr, desc);
    
    if (MEM_WB[PR_READ].instr != NOOP) {
        printf("%8s: %d\t%8s: %d\n",
               "MemToReg", MEM_WB[PR_READ].MemToReg,
               "RegWrite", MEM_WB[PR_READ].RegWrite);
        printf("%11s: 0x%08x\t%11s: 0x%08x\t%11s: %d\n",
               "LWDataValue", MEM_WB[PR_READ].LWDataValue,
               "ALUResult", MEM_WB[PR_READ].ALUResult,
               "WriteRegNum", MEM_WB[PR_READ].WriteRegNum);
    }
	
	printf("==============================================================\n\n");
}
/**
 * Initialize main memory using 0x00–0xFF
 */
void initialize_memory() {
	unsigned char current_value = 0;
	
	for (int n=0; n < MEMORY_SIZE; n++) {
		main_memory[n] = current_value;
		current_value++;
	}
}

/**
 * Initialize registers
 * initial values of x100 plus the register number except for register 0
 */
void initialize_registers() {
	registers[0] = 0;
	
	for (int n=1; n < NUM_REGISTERS; n++) {
		registers[n] = n + 0x100;
	}
	
	/* pipeline registers */
	IF_ID = malloc(sizeof(IF_ID_Reg) * 2);
	IF_ID[PR_WRITE].instr = NOOP;
	IF_ID[PR_READ].instr = NOOP;
	
	ID_EX = malloc(sizeof(ID_EX_Reg) * 2);
	ID_EX[PR_WRITE].instr = NOOP;
	ID_EX[PR_READ].instr = NOOP;
	
	EX_MEM = malloc(sizeof(EX_MEM_Reg) * 2);
	EX_MEM[PR_WRITE].instr = NOOP;
	EX_MEM[PR_READ].instr = NOOP;

    MEM_WB = malloc(sizeof(MEM_WB_Reg) * 2);
    MEM_WB[PR_WRITE].instr = NOOP;
    MEM_WB[PR_READ].instr = NOOP;
}

unsigned int get_opcode(uint32_t instr) {
	unsigned int opcode = (instr & 0xFC000000) >> 26; // 26 - 31
	return opcode;
}

unsigned int get_rs(uint32_t instr) {
	unsigned int rs = (instr & 0x03E00000) >> 21; // 21 - 25
	return rs;
}

unsigned int get_rt(uint32_t instr) {
	unsigned int rt = (instr & 0x001F0000) >> 16; // 16 - 20
	return rt;
}

unsigned int get_rd(uint32_t instr) {
	unsigned int rd = (instr & 0x0000F800) >> 11; // 11 - 15
	return rd;
}

unsigned int get_shamt(uint32_t instr) {
	unsigned int shamt = (instr & 0x000007C0) >> 6; // 6 - 10
	return shamt;
}

unsigned int get_funct(uint32_t instr) {
	unsigned int funct = instr & 0x0000003F; // 0 - 5
	return funct;
}

unsigned int get_immediate(uint32_t instr) {
	short immediate = instr & 0x0000FFFF; // 0 - 15
	return immediate;
}

void desc_instr(uint32_t instr, char *desc) {
	char buffer[18] = "";
	
	if (instr == NOOP) {
		strcat(buffer, "nop");
	} else {
		switch (get_opcode(instr)) {
			case 0x0:
                
                switch (get_funct(instr)) {
                    case 0x20: /* add */
                        sprintf(buffer, "add $%d,$%d,$%d",
                                get_rd(instr),
                                get_rs(instr),
                                get_rt(instr));
                        break;
                        
                    case 0x22: /* sub */
                        sprintf(buffer, "sub $%d,$%d,$%d",
                                get_rd(instr),
                                get_rs(instr),
                                get_rt(instr));
                        break;
                        
                    default:
                        strcpy(buffer, "Unknown funct!");
                        break;
                }
                
                break;
                
			case 0x20: /* lb */ 
                sprintf(buffer, "lb $%d,%d($%d)", 
                        get_rt(instr), 
                        get_immediate(instr), 
                        get_rs(instr));
                break;
                
			case 0x28: /* sb */
                sprintf(buffer, "sb $%d,%d($%d)",
                        get_rt(instr), 
                        get_immediate(instr), 
                        get_rs(instr));
                break;
                
			default:
                strcpy(buffer, "Unknown opcode!");
		}
	}
	
	strcpy(desc, buffer);
}
