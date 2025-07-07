#include <stdio.h>

int main() {
    unsigned int x = 0x12345678;   // 4-byte integer
    unsigned char *c = (unsigned char*)&x;

    printf("Memory layout of x: %02x %02x %02x %02x\n", c[0], c[1], c[2], c[3]);

    if (c[0] == 0x78) {
        printf("Machine is little-endian\n");
    } else if (c[0] == 0x12) {
        printf("Machine is big-endian\n");
    } else {
        printf("Unknown endianness\n");
    }

    return 0;
}

