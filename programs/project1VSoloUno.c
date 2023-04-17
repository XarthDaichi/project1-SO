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

static unsigned char buffer[BUFFERLEN];
static long pos_p = 0;
static long pos_c = 0;

int flag = 1;

static int solution_array[256] = {0};

static int producers_num;
static int consumers_num;

static char *filepath;
static long filelen;
static long consumers_final_pos;

void *reading_file() {
    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    while(!feof(fileptr)) {
        pthread_mutex_lock(&buffer_mutex);
        printf("(1) Locked mutex\n");
        if (!flag) {
            printf("(1) Unlocked mutex: %ld\n", pos_p);
            pthread_mutex_unlock(&buffer_mutex);
            pthread_exit((void*)0);
        }
        fseek(fileptr, pos_p, SEEK_SET);
        fread(buffer+(pos_p%BUFFERLEN), 1, 1, fileptr);
        while(pos_p+1 == pos_c) {
            pthread_cond_wait(&consumed_condition, &buffer_mutex);
        }
        pos_p++;
        if (feof(fileptr)) flag = 0;
        printf("(1) Unlocked mutex: %ld\n", pos_p);
        pthread_mutex_unlock(&buffer_mutex);
        pthread_cond_signal(&read_condition);
    }
    fclose(fileptr);
    pthread_exit((void*)0);
}

void *adding_to_array() {
    while(filelen) {
        pthread_mutex_lock(&solution_mutex);
        printf("(2) Locked mutex\n");
        if(!filelen) {
            printf("(2) Unlocked mutex: %ld\n", pos_c);
            pthread_mutex_unlock(&solution_mutex);
            pthread_exit((void*)0);
        }
        while(pos_c+1 == pos_p) {
            pthread_cond_wait(&read_condition, &solution_mutex);
        }
        solution_array[buffer[pos_c%BUFFERLEN]]++;
        pos_c++;
        filelen--;
        printf("(2) Unlocked mutex: %ld\n", pos_c);
        pthread_mutex_unlock(&solution_mutex);
        pthread_cond_signal(&consumed_condition);
    }
    pthread_exit((void*)0);
}

int main(int argc, char *argv[]) {
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

    for (int i = 0; i < 256; i++) {
        if (solution_array[i] != 0) printf("%d aparece %d veces\n", i, solution_array[i]);
    }

    pthread_mutex_destroy(&solution_mutex);
    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&consumed_condition);
    pthread_cond_destroy(&read_condition);
    return 0;
}