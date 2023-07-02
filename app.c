#include <stdio.h>
#include <stdlib.h>

// http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
// http://6502.org/users/obelisk/6502/reference.html

// VARS AND CONSTS
const unsigned int MAX_MEM = 1024 * 64;
//////

// OPCODES
#define LDA_IM 0xA9
#define LDA_ZP 0xA5
#define LDA_ZPX 0xB5
#define LDA_AB 0xAD
#define JSR 0x20
///////

// FLAGS
const unsigned char CARRY_FLAG = 0;
const unsigned char ZERO_FLAG = 1;
const unsigned char INTERRUPT_FLAG = 2;
const unsigned char DECIMAL_FLAG = 3;
const unsigned char BREAK_FLAG = 4;
const unsigned char RESERVED_FLAG = 5;
const unsigned char OVERFLOW_FLAG = 6;
const unsigned char NEGATIVE_FLAG = 7;
//////

////// STRUCTS
struct Memory
{
    void (*InitMem)(struct Memory *mem);
    unsigned char Data[1024 * 64];
};

struct CPU
{
    unsigned short PC;
    unsigned short SP;

    unsigned char A, X, Y;

    unsigned char FLAGS[8];

    void (*Reset)(struct CPU *cpu, struct Memory *mem);
    void (*Execute)(struct CPU *cpu, struct Memory *mem, unsigned int NCycles);
    unsigned char (*getInstruction)(struct CPU *cpu, struct Memory *mem, unsigned int *NCycles);
    unsigned char (*readByteAtAddr)(struct CPU *cpu, struct Memory *mem, unsigned int *NCycles, unsigned int addr);
    unsigned short (*readBigByte)(struct CPU *cpu, struct Memory *mem, unsigned int *NCycles);
    void (*setLDAFlags)(struct CPU *cpu);
};
//////////

//////// STRUCTS FUNCTIONS
void Reset(struct CPU *cpu, struct Memory *mem)
{
    cpu->PC = 0xFFFC;
    cpu->SP = 0x0100;
    cpu->A = cpu->X = cpu->Y = 0;
    cpu->FLAGS[0] = cpu->FLAGS[1] = cpu->FLAGS[2] = cpu->FLAGS[3] = cpu->FLAGS[4] = cpu->FLAGS[5] = cpu->FLAGS[6] = cpu->FLAGS[7] = 0;

    mem->InitMem(mem);
}

void Execute(struct CPU *cpu, struct Memory *mem, unsigned int NCycles)
{
    while (NCycles > 0)
    {
        unsigned char ins = cpu->getInstruction(cpu, mem, &NCycles);

        switch (ins)
        {
        case LDA_IM:
        {
            unsigned char value = cpu->getInstruction(cpu, mem, &NCycles);
            cpu->A = value;

            cpu->setLDAFlags(cpu);
        }
        break;

        case LDA_ZP:
        {
            unsigned char addr = cpu->getInstruction(cpu, mem, &NCycles);
            cpu->A = cpu->readByteAtAddr(cpu, mem, &NCycles, addr);

            cpu->setLDAFlags(cpu);
        }
        break;

        case LDA_ZPX:
        {
            unsigned char addr = cpu->getInstruction(cpu, mem, &NCycles);
            cpu->A = cpu->readByteAtAddr(cpu, mem, &NCycles, addr + cpu->X);
            // JUST REDUCE CYCLE HERE BECAUSE UP HERE WE MAKE A SUM "addr + cpu->X"
            NCycles--;
            
            cpu->setLDAFlags(cpu);
        }
        break;

        case LDA_AB:
        {
            unsigned short addr = cpu->readBigByte(cpu, mem, &NCycles);
            cpu->A = cpu->readByteAtAddr(cpu, mem, &NCycles, addr);

            cpu->setLDAFlags(cpu); 
        }
        break;

        case JSR:
        {
            unsigned short addr = cpu->readBigByte(cpu, mem, &NCycles);

            mem->Data[cpu->SP] = cpu->PC-1 & 0xFF;
            mem->Data[cpu->SP+1] = (cpu->PC-1 >> 8);
            NCycles-=2;
            cpu->SP+=2;

            cpu->PC = addr;
            NCycles--;
        }
        break;

        default:
        {
            printf("The opcode '%X' not recognized!", ins);
        }
        break;
        }
    }
}

unsigned char getInstruction(struct CPU *cpu, struct Memory *mem, unsigned int *NCycles)
{
    unsigned char data = mem->Data[cpu->PC];
    cpu->PC++;
    (*NCycles)--;
    return data;
}

unsigned char readByteAtAddr(struct CPU *cpu, struct Memory *mem, unsigned int *NCycles, unsigned int addr)
{
    unsigned char data = mem->Data[addr];
    (*NCycles)--;
    return data;
}

unsigned short readBigByte(struct CPU *cpu, struct Memory *mem, unsigned int *NCycles)
{
    unsigned short data = mem->Data[cpu->PC];
    cpu->PC++;
    /// LITTLE ENDIAN
    data = (mem->Data[cpu->PC] << 8) | data;
    cpu->PC++;

    (*NCycles) -= 2;
    return data;
}

void setLDAFlags(struct CPU *cpu){
    cpu->FLAGS[ZERO_FLAG] = (cpu->A == 0);
    cpu->FLAGS[NEGATIVE_FLAG] = (cpu->A & 0b10000000) > 0; 
}

////////////////////////////////////////////////////////////////////////////////////////

void InitMem(struct Memory *mem)
{
    for (unsigned int i = 0; i < MAX_MEM; i++)
    {
        mem->Data[i] = 0;
    }
}
//////////////

int main(int argc, char *argv[])
{
    struct CPU cpu = {0, 0x0100, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}, Reset, Execute, getInstruction, readByteAtAddr, readBigByte, setLDAFlags};
    struct Memory mem = {InitMem};

    cpu.Reset(&cpu, &mem);

    // DEBUG

    cpu.PC = 0xFFFC;
    mem.Data[0xFFFC] = JSR;
    mem.Data[0xFFFD] = 0x42;
    mem.Data[0xFFFE] = 0x42;
    mem.Data[0x4242] = LDA_IM;
    mem.Data[0x4243] = 0x24;
    mem.Data[0x4244] = LDA_AB;
    mem.Data[0x4245] = 0x43;
    mem.Data[0x4246] = 0x42;

    cpu.Execute(&cpu, &mem, 12);

    printf("A: %d\n", cpu.A);
    printf("X: %d\n", cpu.X);
    printf("Y: %d\n\n", cpu.Y);
    
    printf("FLAGS:");
    for(char i=0; i<8; i++){
        printf(" %d", cpu.FLAGS[i]);
    } printf("\n");

    return 0;
}