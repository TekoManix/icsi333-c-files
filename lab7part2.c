#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

void process_file(const char *filename) {
    int fd;
    int numbers[100];
    int sum = 0;
    int i;
    ssize_t bytes_read;
    
    // Open file
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    
    // Read numbers from file
    bytes_read = read(fd, numbers, sizeof(numbers));
    if (bytes_read != sizeof(numbers)) {
        printf("Warning: %s may not contain exactly 100 integers\n", filename);
    }
    
    // Calculate sum
    for (i = 0; i < 100; i++) {
        sum += numbers[i];
    }
    
    // Print filename and sum
    printf("%s: %d\n", filename, sum);
    
    // Close file
    close(fd);
}

int main(void) {
    DIR *dir;
    struct dirent *entry;
    
    // Open current directory
    dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening directory");
        return 1;
    }
    
    // Loop through directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Check if filename starts with "numbers."
        if (strncmp(entry->d_name, "numbers.", 8) == 0) {
            process_file(entry->d_name);
        }
    }
    
    // Close directory
    closedir(dir);
    
    return 0;
}
