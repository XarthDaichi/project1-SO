#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define BUFFERLEN 1000

//mutex
static pthread_mutex_t solution_mutex[256];
static pthread_mutex_t buffer_mutex[BUFFERLEN];
static pthread_mutex_t bytes_consumer_mutex = PTHREAD_MUTEX_INITIALIZER;

//conditions
static pthread_cond_t read_condition[BUFFERLEN];
static pthread_cond_t consumed_condition[BUFFERLEN];
int read[BUFFERLEN];
int end_of_file_flag = 1;

//buffer for reading
static unsigned char buffer[BUFFERLEN];

//array where things are going to get counted
static int solution_array[256];

//inputs
static int producers_num = 0;
static int consumers_num = 0;

//file information
static char *filepath;
static long filelen;
static long bytes_consumers;

void *reading_file(void *thread_index) {
    int index = *((int *)thread_index);
    long file_reader_pos = index;
    int buffer_adder_pos = index;

    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    while(!feof(fileptr)) {
        if (buffer_adder_pos >= BUFFERLEN) buffer_adder_pos = index;
        fseek(fileptr, file_reader_pos, SEEK_SET);
        pthread_mutex_lock(&buffer_mutex[buffer_adder_pos]);
        while(read[buffer_adder_pos]) {
            pthread_cond_wait(&consumed_condition[buffer_adder_pos], &buffer_mutex[buffer_adder_pos]);
        }
        fread(buffer+buffer_adder_pos, 1, 1, fileptr);
        read[buffer_adder_pos] = 1;
        printf("%s", &buffer);
        pthread_mutex_unlock(&buffer_mutex[buffer_adder_pos]);
        pthread_cond_broadcast(&read_condition[buffer_adder_pos]);
        file_reader_pos += producers_num;
        buffer_adder_pos += producers_num;
    }
    fclose(fileptr);
    end_of_file_flag = 0;
}

void *adding_to_array(void *thread_index) {
    int index = *((int *)thread_index);
    int buffer_consumer_pos = index;
    // printf("%d: Started Consuming\n", index);
    while(end_of_file_flag || bytes_consumers) {
        if (buffer_consumer_pos >= BUFFERLEN) buffer_consumer_pos = index;
        pthread_mutex_lock(&buffer_mutex[buffer_consumer_pos]);
        while (!read[buffer_consumer_pos] && bytes_consumers) {
            pthread_cond_wait(&read_condition[buffer_consumer_pos], &buffer_mutex[buffer_consumer_pos]);
        }
        // printf("%d: Finished waiting\n");
        pthread_mutex_lock(&bytes_consumer_mutex);
        bytes_consumers--;
        pthread_mutex_unlock(&bytes_consumer_mutex);
        pthread_mutex_lock(&solution_mutex[buffer_consumer_pos]);
        solution_array[buffer[buffer_consumer_pos]]++;
        buffer[buffer_consumer_pos] = '\000';
        pthread_mutex_unlock(&solution_mutex[buffer_consumer_pos]);
        read[buffer_consumer_pos] = 0;
        pthread_mutex_unlock(&buffer_mutex[buffer_consumer_pos]);
        pthread_cond_broadcast(&consumed_condition[buffer_consumer_pos]);
    }
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < BUFFERLEN; i++) {
        if (i < 256) {
            pthread_mutex_init(&solution_mutex[i], NULL);
            solution_array[i] = 0;
        }
        pthread_mutex_init(&buffer_mutex[i], NULL);
        pthread_cond_init(&consumed_condition[i], NULL);
        pthread_cond_init(&read_condition[i], NULL);
        read[i] = 0;
    }
    
    if (argc > 1) producers_num = atoi(argv[1]);
    else {
        printf("You fucked up!");
        return -1;
    }

    if (argc > 2) consumers_num = atoi(argv[2]);
    else {
        printf("You fucked up x2!");
        return -1;
    }

    if (argc > 3) filepath = (argv[3]);
    else {
        printf("You fucked up x3!");
        return -1;
    }
    pthread_t producers[producers_num];
    pthread_t consumers[consumers_num];

    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    bytes_consumers = filelen;
    rewind(fileptr);
    fclose(fileptr);
    int larger;
    larger = (consumers_num > producers_num) ? consumers_num : producers_num;
    int index[larger];
    for (int i = 0; i < larger; i++) index[i] = i;
    for (int i = 0; i < producers_num; i++) {
        if (pthread_create(&producers[i], NULL, reading_file, &index[i]) != 0) {
            printf("Thread creation fucked up!: consumers");
            return -1;
        }
    }

    int visited_p[producers_num];
    for (int i = 0; i < producers_num; i++) visited_p[i] = 0;
    while(bytes_consumers > 0) {
        for (int i = 0; i < consumers_num; i++) {
            if (pthread_create(&consumers[i], NULL, adding_to_array, &index[i]) != 0) {
                printf("Thread creation fucked up!: consumers");
                return -1;
            }
        }
        for (int i = 0; i < producers_num; i++) {
            if (!visited_p[i]) {
                if (pthread_join(producers[i], NULL) == 0) {
                    visited_p[i] = 1;
                }
            }
        }
        for (int i = 0; i < consumers_num; i++) {
            if (pthread_join(consumers[i], NULL) != 0) {
            }
        }
    }
    printf("Finished\n");
    for (int i = 0; i < BUFFERLEN; i++) {
        if (i < 256) {
            pthread_mutex_destroy(&solution_mutex[i]);
            if (solution_array[i] != 0) printf("%d aparece %d veces\n", i, solution_array[i]);
        }
        pthread_mutex_destroy(&buffer_mutex[i]);
        pthread_cond_destroy(&consumed_condition[i]);
        pthread_cond_destroy(&read_condition[i]);
    }
    printf("%s", &buffer);
}
