#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TO_HEX(i) (i <= 9 ? '0' + i : 'a' - 10 + i)
#define PC 8
#define IR 9
#define LD 2
#define LEA 14
#define LDI 10
#define AND 5
#define ADD 1
#define NOT 9
#define BR 0


char * toHex(int val);
void print_format(int * registers, int cc_val);
int set_cc(int value);

int main(int argc, char** argv) {
    FILE *file;

    short *buffer;
    unsigned long fileLen;
    int registers[11] = {0};

    int registerAddress;
    int opcode;
    int cc;
    int offset;

    file = fopen(argv[1], "rb");
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    rewind(file);

    buffer = malloc(fileLen);

    fread(buffer, fileLen, 1, file);
    fclose(file);


    int i = 0;
    for (i = 0; i < fileLen; i++) {
        buffer[i] = ((buffer[i] & 0x00FF) << 8 | ((buffer[i] & 0xFF00) >> 8));
    }

    registers[8] = buffer[0];
    int clock = 1;
    opcode = (buffer[clock] & 0xf000) >> 12;
    while (opcode != 15 && (clock - 1) != fileLen) {
        int pc_counter = clock + 1;
        opcode = (buffer[clock] & 0xf000) >> 12;
        switch (opcode) {
            case LD:
                registers[PC] = registers[PC] + clock;
                registerAddress = (buffer[clock] & 0x0e00) >> 9;
                offset = buffer[clock] & 0x01ff;
                registers[registerAddress] = buffer[1 + clock + offset];
                registers[IR] = buffer[clock];
                cc = set_cc(registers[registerAddress]);
                break;
            case LEA:
                registerAddress = (buffer[clock] & 0x0e00) >> 9;
                offset = buffer[clock] & 0x01ff;
                registers[PC] = registers[8] + 1;
                registers[IR] = buffer[clock];
                registers[registerAddress] = registers[PC] + offset;
                cc = set_cc(registers[registerAddress]);
                break;
            case LDI:
                registers[PC] = registers[PC] + 1;
                registers[IR] = buffer[clock];
                registerAddress = (buffer[clock] & 0x0e00) >> 9;
                int address_offset = pc_counter - clock + 1;
                int l;
                for (l = 0; l < fileLen; l++) {
                    registers[registerAddress] = buffer[address_offset];
                    cc = set_cc(registers[registerAddress]);
                    break;
                }
                break;
            case AND:
                registers[PC] = registers[8] + 1;
                registers[IR] = buffer[clock];
                int immediate = (buffer[clock] & 0x20) >> 5;
                int DR = (buffer[clock] & 0x0e00) >> 9;
                if (immediate == 1) {
                    int SR = (buffer[clock] & 0x01c0) >> 9;
                    int imm5 = (buffer[clock] & 0x1f);
                    registers[DR] = registers[SR] & imm5;
                    cc = set_cc(registers[DR]);
                } else if (immediate == 0) {
                    int SR1 = (buffer[clock] & 0x01c0) >> 9;
                    int SR2 = (buffer[clock] & 0x7);
                    registers[DR] = registers[SR1] & registers[SR2];
                    cc = set_cc(registers[DR]);
                }
                break;
            case NOT:
                registers[8] = registers[8] + 1;
                registers[9] = buffer[clock];
                DR = (buffer[clock] & 0x0e00) >> 9;
                int SR = (buffer[clock] & 0x01c0) >> 6;
                registers[DR] = ~registers[SR];
                cc = set_cc(~registers[SR]);
                break;
            case ADD:
                registers[PC] = registers[PC] + 1;
                registers[IR] = buffer[clock];
                immediate = (buffer[clock] & 0x20) >> 5;
                DR = (buffer[clock] & 0x0e00) >> 9;
                if (immediate == 1) {
                    int SR = (buffer[clock] & 0x1c0) >> 6;
                    int imm5 = (buffer[clock] & 0x1f);
                    registers[DR] = registers[SR] + imm5;
                    cc = set_cc(registers[DR]);
                } else if (immediate == 0) {
                    int SR1 = (buffer[clock] & 0x01c0) >> 9;
                    int SR2 = (buffer[clock] & 0x7);
                    registers[DR] = registers[SR1] + registers[SR2];
                    cc = set_cc(registers[DR]);
                }
                break;
            case BR:
                registers[PC] = registers[PC] + 1;
                registers[IR] = buffer[clock];
                int n = (buffer[clock] & 0x0800) >> 9;
                int z = (buffer[clock] & 0x0400) >> 9;
                int p = (buffer[clock] & 0x0200) >> 9;
                if (((n != 0) && cc == -1) || ((z != 0) && cc == 0) || ((p != 0) && cc == 1)) {
                    offset = buffer[clock] & 0x01ff;
                    registers[PC] = registers[PC] + offset;
                }
                printf("after executing instruction\t0x%s\n", toHex(registers[IR]));
                print_format(registers, cc);
                printf("==================");
                break;
        }
        clock++;
    }
    return 0;
}

int set_cc(int value) {
    if (value < 0)
        return -1;
    if (value == 0)
        return 0;
    return 1;
}

void print_format(int * registers, int cc_val) {
    int i;
    for (i = 0; i < 11; i++) {
        if (i == PC)
            printf("PC\t0x%s\n", toHex(registers[PC]));
        else if (i == IR)
            printf("IR\t0x%s\n", toHex(registers[IR]));
        else if (i == 10) {
            if (cc_val == -1)
                printf("CC\t%s\n", "N");
            if (cc_val == 0)
                printf("CC\t%s\n", "Z");
            if (cc_val == 1)
                printf("CC\t%s\n", "P");
        } else
            printf("R%d\t0x%s\n", i, toHex(registers[i]));
    }
}

char * toHex(int val) {
    int x = val;
    char *res = (char *) malloc(sizeof (char) * 5);
    if (x <= 0xffff) {
        res[0] = TO_HEX(((x & 0xf000) >> 12));
        res[1] = TO_HEX(((x & 0x0f00) >> 8));
        res[2] = TO_HEX(((x & 0x00f0) >> 4));
        res[3] = TO_HEX((x & 0x000f));
        res[4] = '\0';
    }
    return res;
}