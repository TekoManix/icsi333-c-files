#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    int numbers[100];
    int sum = 0;
    int i;
    char filename[50];
    int fd;
    
    // Seed random number generator
    srand(time(0));
    
    // Generate random numbers and calculate sum
    for (i = 0; i < 100; i++) {
        numbers[i] = rand() % 100;  // Random number 0-99
        sum += numbers[i];
    }
    
    // Print the sum
    printf("Sum: %d\n", sum);
    
    // Create filename with sum
    sprintf(filename, "numbers.%d", sum);
    
    // Open file for writing (create if doesn't exist, truncate if exists)
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }
    
    // Write numbers to file
    write(fd, numbers, sizeof(numbers));
    
    // Close file
    close(fd);
    
    printf("Numbers written to %s\n", filename);
    
    return 0;
}
