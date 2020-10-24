#include <stdio.h>
#include <stdint.h>

void dummy (uint32_t i)
{
    printf("%u\n", i);
}

int main ()
{
    uint32_t i = 10;
    dummy(i);
}
