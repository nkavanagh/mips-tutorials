/**
 * Niall Kavanagh <niall@kst.com>
 * Compiled and run on OS X
 * Disassemble MIPS instructions
 */

#include <stdio.h>
#include <stdint.h>

void print_inst(uint32_t bits, uint32_t addr) {
	//printf("Instruction at %x: %x\n", addr, bits);
	
	unsigned int opcode = (bits & 0xFC000000) >> 26; // 26 - 31
	
	/* for r-type */
	unsigned int rs = (bits & 0x03E00000) >> 21; // 21 - 25
	unsigned int rt = (bits & 0x001F0000) >> 16; // 16 - 20
	unsigned int rd = (bits & 0x0000F800) >> 11; // 11 - 15
	unsigned int shamt = (bits & 0x000007C0) >> 6; // 6 - 10
	unsigned int funct = bits & 0x0000003F; // 0 - 5
	
	/* for i-type */
	short immediate = bits & 0x0000FFFF; // 0 - 15 
	
	/* jump address is counter + offset shifted left by 2 */
	unsigned int dest_addr = addr + (immediate << 2);
	
	/* for j-type (currently unused) */
	unsigned int address = bits & 0x03FFFFFF; // 0 - 25
	
	switch (opcode) {
		case 0x0:
		
		switch (funct) {
			case 0x20: /* add */
			printf("%x\tadd $%d,$%d,$%d\n", addr, rd, rs, rt);
			break;
			
			case 0x22: /* sub */
			printf("%x\tsub $%d,$%d,$%d\n", addr, rd, rs, rt);
			break;
			
			case 0x24: /* and */
			printf("%x\tand $%d,$%d,$%d\n", addr, rd, rs, rt);
			break;
			
			case 0x25: /* or */
			printf("%x\tor $%d,$%d,$%d\n", addr, rd, rs, rt);
			break;
			
			case 0x2a: /* slt */
			printf("%x\tslt $%d,$%d,$%d\n", addr, rd, rs, rt);
			break;
			
			default:
			printf("%x\tUnknown funct %x for opcode %x (%x)\n", addr,funct, opcode, bits);
			break;
		}
		
		break;
		
		case 0x23: /* lw */ // TODO
		printf("%x\tlw $%d,%d($%d)\n", addr, rt, immediate, rs);
		break;
		
		case 0x2b: /* sw */
		printf("%x\tsw $%d,%d($%d)\n", addr, rt, immediate,rs);
		break;
		
		case 0x4: /* beq */
		printf("%x\tbeq $%d,$%d, address %x\n", addr, rs, rt, dest_addr);
		break;
		
		case 0x5: /* bne */
		printf("%x\tbne $%d,$%d, address %x\n", addr, rs, rt, dest_addr);
		break;
		
		default:
		printf("%x\tUnknown opcode: %x (%x)\n", addr, opcode, bits);
	}
}

/* main */
int main(int argc, char *argv[]) {
	uint32_t instructions[] = { 0x022DA822, 
		0x12A70003, 
		0x8D930018, 
		0x02689820, 
		0xAD930018, 
		0x02697824, 
		0xAD8FFFF4, 
		0x018C6020, 
		0x02A4A825, 
		0x158FFFF6, 
		0x8E59FFF0};
		
	size_t s = sizeof(instructions)/sizeof(uint32_t);
	uint32_t addr = 0x7a060;
	
	printf("Disassembling %ld instructions. Base address is %x.\n", s, addr);

	for (int i=0; i < s; i++) {
		print_inst(instructions[i], addr);
		addr += sizeof(uint32_t);
	}
    return 0;
}
