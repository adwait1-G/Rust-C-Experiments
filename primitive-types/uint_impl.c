#include <stdio.h>
#include <stdint.h>

void dummy(uint32_t x);
uint32_t from_ne_bytes(uint8_t *arr);

int main()
{
    uint8_t arr[] = {0x12, 0x34, 0x56, 0x78};
    uint32_t val = from_ne_bytes(arr);
    dummy(val);
}

uint32_t from_ne_bytes (uint8_t *arr)
{
    return *(uint32_t *)(arr);
}

void dummy (uint32_t x)
{
    printf("%x\n", x);
}
