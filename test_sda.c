
#include <stdio.h>

#define sdanew(s, init, init_sz) ({\
    char tmp[] = {1, 2, 3, 4, 5, 6, 7, 8}; \
    void *ptr = (void *)tmp; \
    (typeof(s))ptr; \
    })

int main(int argc, char* argv[])
{
    // int *sda;
    // char tmp[] = {1, 2, 3, 4, 5, 6, 7, 8};
    // void *ptr = (void *)tmp;
    // sda = (typeof(sda))ptr;
    
    int *sda = sdanew(sda, 0, 0);
    printf("0x%x\n", sda[0]);
}
