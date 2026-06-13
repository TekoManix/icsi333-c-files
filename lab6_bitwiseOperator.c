#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <integer>\n", argv[0]);
        return 1;
    }

    int number = atoi(argv[1]);

    printf("Your number was %d\n", number);

    int bits_zero = 0;
    int bits_one = 0;

    for (int i = 0; i < 32; i++)
    {
        if ((number & (1 << i)) != 0)
        {
            bits_one++;
        }
        else
        {
            bits_zero++;
        }
    }

    printf("In %d, there are %d bits set to 0.\n", number, bits_zero);
    printf("In %d, there are %d bits set to 1.\n", number, bits_one);
    
    return 0;
}