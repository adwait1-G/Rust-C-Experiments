#include <stdio.h>
#include <stdint.h>

#define ARR_SIZE 100

void dummy(int64_t *arr);

int main ()
{
    int64_t arr[ARR_SIZE] = {0};
    dummy(arr);

    return 0;
}

void dummy (int64_t *arr)
{   
    int i = 0;
    for(i = 0; i < ARR_SIZE; i++)
    {
        printf("%d ", arr[i]);
    }
}
