#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define SHM_NAME "/myArea"
#define FILE_SIZE 10000
#define OUTPUT_FILE "output_data"

int main() {
    int shm_fd;
    void *shared_mem;
    int out_fd;
    void *mapped_output;
    
    // Open shared memory object
    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    
    // Map shared memory
    shared_mem = mmap(NULL, FILE_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap shared memory");
        close(shm_fd);
        exit(1);
    }
    
    // Create and open output file
    out_fd = open(OUTPUT_FILE, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (out_fd == -1) {
        perror("open output file");
        munmap(shared_mem, FILE_SIZE);
        close(shm_fd);
        exit(1);
    }
    
    // Set the size of output file
    if (ftruncate(out_fd, FILE_SIZE) == -1) {
        perror("ftruncate output file");
        munmap(shared_mem, FILE_SIZE);
        close(shm_fd);
        close(out_fd);
        exit(1);
    }
    
    // mmap the output file
    mapped_output = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, out_fd, 0);
    if (mapped_output == MAP_FAILED) {
        perror("mmap output file");
        munmap(shared_mem, FILE_SIZE);
        close(shm_fd);
        close(out_fd);
        exit(1);
    }
    
    // Copy data from shared memory to mmap'ed output file
    memcpy(mapped_output, shared_mem, FILE_SIZE);
    
    printf("Data copied from shared memory to %s successfully.\n", OUTPUT_FILE);
    
    // Cleanup
    munmap(shared_mem, FILE_SIZE);
    munmap(mapped_output, FILE_SIZE);
    close(shm_fd);
    close(out_fd);
    
    // Clean up shared memory
    shm_unlink(SHM_NAME);
    
    return 0;
}
