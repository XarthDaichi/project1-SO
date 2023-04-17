#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define BUFFERLEN 1000

static pthread_mutex_t solution_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t read_condition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t consumed_condition = PTHREAD_COND_INITIALIZER;

int read[BUFFERLEN];

static unsigned char buffer[BUFFERLEN];
static int pos_p = 0;
static int pos_c = 0;

static int solution_array[256];

static int producers_num;
static int consumers_num;

static cahr *filepath;
static long filelen;
static long bytes_consumers;

void *reading_file() {
    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    for (int i = 0; i < filelen; i++) {
        fseek(fileptr, i, SEEK_SET);
        pthread_mutex_lock(&buffer_mutex);
        while(read[i]) {
            pthread_cond_wait(&consumed_condition);
        }
        fread(buffer+pos_p, 1, 1, fileptr);
        read[j] = 1;
        pthread_mutex_unlock(&buffer_mutex);
        pthread_cond_broadcast(&read_condition);
    }
    close(fileptr);
}

void *adding_to_array() {

}

int main(int argc, char *argv[]) {
    for (int i = 0; i < BUFFERLEN; i++) {
        if (i < 256) {
            solution_array[i] = 0;
        }
        read[i] = 0;
    }
    
    if (argc > 1) producers_num = atoi(argv[1]);
    else {
        printf("Falta el numero de productores");
        return -1;
    }

    if (argc > 2) consumers_num = atoi(argv[2]);
    else {
        printf("Falta el numero de consumidores");
        return -1;
    }

    if (argc > 3) filepath = (argv[3]);
    else {
        printf("Falta la dirección del archivo");
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
    for (int i = 0; i < producers_num; i++) {
        if (pthread_create(&producers[i], NULL, reading_file, NULL) != 0) {
            printf("No se pudo crear el hilo de productores");
            return -1;
        }
    }

    for (int i = 0; i < consumers_num; i++) {
        if (pthread_create(&consumers[i], NULL, adding_to_array, NULL) != 0) {
            printf("No se pudo crear el hilo de consumidores");
            return -1;
        }
    }

    for (int i = 0; i < producers_num; i++) {
        if (pthread_join(producers[i], NULL)) {
            printf("No se pudo unir el hilo de productores");
            return -1;
        }
    }

    for (int i = 0; i < consumers_num; i++) {
        if (pthread_join(consumers[i], NULL)) {
            printf("No se pudo unir el hilo de consumidores");
            return -1;
        }
    }

    for (int i = 0; i < 256; i++) {
        if (solution_array[i] != 0) printf("%d aparece %d veces\n", i, solution_array[i]);
    }

    pthread_mutex_destroy(&solution_mutex);
    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&consumed_condition);
    pthread_cond_destroy(&read_condition);



    return 0;
}