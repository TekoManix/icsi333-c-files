#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

/* Global pipes */
int pipe1[2];  /* ping writes, pong reads */
int pipe2[2];  /* pong writes, ping reads */

/* Signal handler function */
void signal_handler(int sig) {
    printf("pong quitting\n");
    exit(0);
}

/* Ping function - runs in child process 1 */
void ping_process() {
    int value = 0;
    
    /* Close unused pipe ends */
    close(pipe1[0]);  /* Close read end of pipe1 */
    close(pipe2[1]);  /* Close write end of pipe2 */
    
    while (value < 100) {
        printf("ping - %d\n", value);
        value++;
        
        /* Write to pipe1 */
        write(pipe1[1], &value, sizeof(int));
        
        /* Read from pipe2 */
        read(pipe2[0], &value, sizeof(int));
    }
    
    /* Close remaining pipe ends */
    close(pipe1[1]);
    close(pipe2[0]);
    
    exit(0);
}

/* Pong function - runs in child process 2 */
void pong_process() {
    int value;
    
    /* Set up signal handler for SIGUSR1 */
    signal(SIGUSR1, signal_handler);
    
    /* Close unused pipe ends */
    close(pipe1[1]);  /* Close write end of pipe1 */
    close(pipe2[0]);  /* Close read end of pipe2 */
    
    /* Loop forever */
    while (1) {
        /* Read from pipe1 */
        if (read(pipe1[0], &value, sizeof(int)) <= 0) {
            break;
        }
        
        printf("pong - %d\n", value);
        value++;
        
        /* Write to pipe2 */
        write(pipe2[1], &value, sizeof(int));
    }
    
    /* Close remaining pipe ends */
    close(pipe1[0]);
    close(pipe2[1]);
    
    exit(0);
}

int main() {
    pid_t pid1, pid2;
    
    /* Create two pipes */
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(1);
    }
    
    /* Fork first child for ping */
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(1);
    }
    
    if (pid1 == 0) {
        /* Child 1 - ping process */
        ping_process();
    }
    
    /* Fork second child for pong */
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(1);
    }
    
    if (pid2 == 0) {
        /* Child 2 - pong process */
        pong_process();
    }
    
    /* Parent process */
    /* Close all pipe ends in parent */
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);
    
    /* Wait for ping process to finish */
    waitpid(pid1, NULL, 0);
    
    /* Send SIGUSR1 signal to pong process */
    kill(pid2, SIGUSR1);
    
    /* Wait for pong process to finish */
    waitpid(pid2, NULL, 0);
    
    printf("Program complete\n");
    
    return 0;
}
