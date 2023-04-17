#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define BUFFERLEN 1000

static pthread_mutex_t solution_mutex;
static pthread_mutex_t buffer_mutex;
static pthread_mutexattr_t buffer_mutex_attr;

static pthread_cond_t read_condition;
static pthread_cond_t consumed_condition;

int read[BUFFERLEN];

static unsigned char buffer[BUFFERLEN];
static int pos_p = 0;
static int pos_c = 0;

int flag = 1;

static int solution_array[256];

static int producers_num;
static int consumers_num;

static char *filepath;
static long filelen;
static long filepos;
static long bytes_consumers;

void *reading_file() {
    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    while(!feof(fileptr)) {
        pthread_mutex_lock(&buffer_mutex);
        if (pos_p >= BUFFERLEN) pos_p = 0;
        while(read[pos_p]) {
            pthread_cond_wait(&consumed_condition, &buffer_mutex);
        }
        fseek(fileptr, filepos, SEEK_SET);
        fread(buffer+pos_p, 1, 1, fileptr);
        bytes_consumers++;
        pos_p++;
        filepos++;
        read[pos_p] = 1;
        printf("%s\n", &buffer);
        pthread_mutex_unlock(&buffer_mutex);
        pthread_cond_broadcast(&read_condition);
    }
    fclose(fileptr);
    flag = 0;
}

void *adding_to_array() {
    while(flag || bytes_consumers != 0) {
        pthread_mutex_lock(&buffer_mutex);
        if (pos_c >= BUFFERLEN) pos_c = 0;
        while(!read[pos_c]) {
            pthread_cond_wait(&read_condition, &buffer_mutex);
        }
        solution_array[buffer[pos_c]]++;
        pos_c++;
        bytes_consumers--;
        read[pos_c] = 0;
        pthread_mutex_unlock(&buffer_mutex);
        pthread_cond_broadcast(&consumed_condition);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < BUFFERLEN; i++) {
        if (i < 256) {
            solution_array[i] = 0;
        }
        // pthread_cond_init(&read_condition[i], NULL);
        // pthread_cond_init(&consumed_condition[i], NULL);
        read[i] = 0;
    }
    pthread_mutexattr_init(&buffer_mutex_attr);
    pthread_mutexattr_settype(&buffer_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&buffer_mutex, NULL);
    pthread_mutex_init(&solution_mutex, NULL);

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
    if (fileptr == NULL) {
        printf("Estuvo mal la abrida de documento");
        return -1;
    }
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    fclose(fileptr);
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

    // printf("%s\n", &buffer);

    // for (int i = 0; i < 256; i++) {
    //     if (solution_array[i] != 0) printf("%d aparece %d veces\n", i, solution_array[i]);
    // }

    pthread_mutex_destroy(&solution_mutex);
    pthread_mutex_destroy(&buffer_mutex);
    pthread_mutexattr_destroy(&buffer_mutex_attr);
    pthread_cond_destroy(&consumed_condition);
    pthread_cond_destroy(&read_condition);



    return 0;
}