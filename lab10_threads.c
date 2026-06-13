#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define ARRAY_SIZE 1000000
#define NUM_THREADS 2

int *numbers;
long long global_sum = 0;
sem_t semaphore;

typedef struct 
{
    int start_index;
    int end_index;
    int use_semaphore;
    int use_local_var;
} thread_data_t;

void* sum_array(void* arg) 
{
    thread_data_t *data = (thread_data_t*)arg;
    long long local_sum = 0;
    
    if (data->use_local_var) 
	{
        // Sum using local variable first
        for (int i = data->start_index; i < data->end_index; i++) 
		{
            local_sum += numbers[i];
        }
        
        // Then update global with semaphore protection
        sem_wait(&semaphore);
        global_sum += local_sum;
        sem_post(&semaphore);
    } 
	else if (data->use_semaphore) 
	{
        // Sum directly to global with semaphore on each access
        for (int i = data->start_index; i < data->end_index; i++) 
		{
            sem_wait(&semaphore);
            global_sum += numbers[i];
            sem_post(&semaphore);
        }
    } 
	else 
	{
        // Sum directly to global without any protection (race condition)
        for (int i = data->start_index; i < data->end_index; i++) 
		{
            global_sum += numbers[i];
        }
    }
    
    return NULL;
}

long long single_threaded_sum() 
{
    long long sum = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) 
	{
        sum += numbers[i];
    }
    return sum;
}

void run_test(int use_semaphore, int use_local_var, const char* description) 
{
    global_sum = 0;
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    int chunk_size = ARRAY_SIZE / NUM_THREADS;
    
    for (int i = 0; i < NUM_THREADS; i++) 
	{
        thread_data[i].start_index = i * chunk_size;
        thread_data[i].end_index = (i + 1) * chunk_size;
        thread_data[i].use_semaphore = use_semaphore;
        thread_data[i].use_local_var = use_local_var;
        
        pthread_create(&threads[i], NULL, sum_array, &thread_data[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) 
	{
        pthread_join(threads[i], NULL);
    }
    
    printf("%s: %lld\n", description, global_sum);
}

int main() 
{
    srand(time(NULL));
    
    // Allocate and initialize array
    numbers = (int*)malloc(ARRAY_SIZE * sizeof(int));
    if (numbers == NULL) 
	{
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    for (int i = 0; i < ARRAY_SIZE; i++) 
	{
        numbers[i] = rand() % 10;
    }
    
    // Initialize semaphore
    sem_init(&semaphore, 0, 1);
    
    // Single-threaded sum (correct answer)
    long long correct_sum = single_threaded_sum();
    printf("Single-threaded sum (correct): %lld\n\n", correct_sum);
    
    // Test 1: Two threads without semaphore (race condition)
    printf("Test 1: Race Condition (no semaphore) \n");
    run_test(0, 0, "Threaded sum");
    
    // Test 2: Two threads with semaphore on each access
    printf("\nTest 2: Semaphore on Each Access \n");
    run_test(1, 0, "Threaded sum");
    
    // Test 3: Two threads with local variable and semaphore
    printf("\nTest 3: Local Variable + Semaphore \n");
    run_test(0, 1, "Threaded sum");
    
    // Cleanup
    sem_destroy(&semaphore);
    free(numbers);
    
    return 0;
}
