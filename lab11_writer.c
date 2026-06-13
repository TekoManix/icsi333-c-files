#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define SHM_NAME "/myArea"
#define FILE_SIZE 10000

int main() {
    int fd;
    void *mapped_file;
    void *shared_mem;
    int shm_fd;
    
    // Open the data file
    fd = open("data", O_RDONLY);
    if (fd == -1) {
        perror("open data file");
        exit(1);
    }
    
    // mmap the data file into memory
    mapped_file = mmap(NULL, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_file == MAP_FAILED) {
        perror("mmap data file");
        close(fd);
        exit(1);
    }
    
    // Create shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        munmap(mapped_file, FILE_SIZE);
        close(fd);
        exit(1);
    }
    
    // Set the size of shared memory
    if (ftruncate(shm_fd, FILE_SIZE) == -1) {
        perror("ftruncate");
        munmap(mapped_file, FILE_SIZE);
        close(fd);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(1);
    }
    
    // Map shared memory
    shared_mem = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap shared memory");
        munmap(mapped_file, FILE_SIZE);
        close(fd);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(1);
    }
    
    // Copy data from mmap'ed file to shared memory
    memcpy(shared_mem, mapped_file, FILE_SIZE);
    
    printf("Data copied to shared memory successfully.\n");
    printf("Shared memory available at /dev/shm%s\n", SHM_NAME);
    
    // Cleanup
    munmap(mapped_file, FILE_SIZE);
    munmap(shared_mem, FILE_SIZE);
    close(fd);
    close(shm_fd);
    
    return 0;
}
