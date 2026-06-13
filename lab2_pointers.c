#include <stdio.h>

// Function that increments an integer by value
// This function receives a COPY of the value, so changes don't affect the original
void increment(int x) { 
    x++;
}

// Function that increments an integer by reference (using a pointer)
// This function receives the ADDRESS of the variable, so it can modify the original
void incrementByReference(int *x) { 
    (*x)++; 
}

int main() {
    int a = 0;
    int b = 0;
    
    int *pa = &a;
    int *pb = &b;
    
    a = 5;
    
    *pb = 6;
    
    printf("Initial values:\n");
    printf("a = %d, b = %d, *pa = %d, *pb = %d\n", a, b, *pa, *pb);
    printf("pa points to address: %p, pb points to address: %p\n\n", (void*)pa, (void*)pb);
    
    printf("Calling increment(b) - pass by value:\n");
    increment(b);
    printf("After increment(b): a = %d, b = %d, *pa = %d, *pb = %d\n\n", a, b, *pa, *pb);
    
    printf("Calling incrementByReference(pb) - pass by reference:\n");
    incrementByReference(pb);
    printf("After incrementByReference(pb): a = %d, b = %d, *pa = %d, *pb = %d\n\n", a, b, *pa, *pb);
    
    /* 
     * EXPLANATION OF THE DIFFERENCE:
     * 
     * increment(b) - Pass by Value:
     * - The function receives a COPY of b's value
     * - Changes inside the function only affect the copy
     * - The original variable b remains unchanged
     * 
     * incrementByReference(pb) - Pass by Reference:
     * - The function receives the ADDRESS where b is stored (through pointer pb)
     * - The function can access and modify the original memory location
     * - Changes inside the function affect the original variable b
     * 
     * This is why b didn't change after increment(b) but DID change after incrementByReference(pb)
     */
    
    return 0;
}