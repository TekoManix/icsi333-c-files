#include <stdio.h>
#include <string.h>

int main() {
    char input_string[1000];  // Buffer to store the input string
    int manual_length = 0;    // Variable to store our calculated length
    
    // Read string from user
    printf("Enter a string: ");
    scanf("%s", input_string);
    
    // Calculate length manually without using standard functions
    // We iterate through each character until we find the null terminator (0 or '\0')
    while (input_string[manual_length] != '\0') {
        manual_length++;
    }
    
    // Print our calculated length and the standard library length
    printf("Calculated length: %d\n", manual_length);
    printf("Standard function length: %lu\n", strlen(input_string));
    
    return 0;
}