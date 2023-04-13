#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int productores_num = 0;
static int consumidores_num = 0;
static char *filepath;
static long filelen;
static int array_bytes[256];


void errorExit(char *strerr) {
    perror(strerr);
    exit(1);
}

void *read_file(void *thread_index) {
    FILE *fileptr;
    char *buffer;
    long readlen, totallen;
    int index = *((int *)thread_index), read_size = filelen/productores_num;
    printf("Comenzando el thread: %d\n", index);
    fileptr = fopen(filepath, "rb");
    if (index != productores_num-1) {
        fseek(fileptr, read_size*index, read_size*index+1);
    } else {
        fseek(fileptr, read_size*index, SEEK_END);
    }
    readlen = ftell(fileptr);
    rewind(fileptr);
    buffer = (char *) malloc(readlen * sizeof(char));
    fread(buffer, readlen, 1, fileptr);
    for (int i = 0; i < readlen; i++) {
        printf("%c", buffer[i]);
    }
    fclose(fileptr);
    printf("Finalizando thread: %d\n", index);
}

void *modify_array(void *modifying_byte) {
    char _modifying_byte = *((char*)modifying_byte);
    pthread_mutex_lock(&mutex);
    array_bytes[_modifying_byte]++;
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char* argv[]) {
    if (argc > 1) productores_num = atoi(argv[1]);
    else errorExit("Argumento de entrada faltante, número de productores\n");

    if (argc > 2) consumidores_num = atoi(argv[2]);
    else errorExit("Argumento de entrada faltante, número de consumidores\n");

    if (argc > 3) filepath = (argv[3]);
    else errorExit("Argumento de entrada faltante, no hay archivo\n");
    
    pthread_t productores[productores_num];
    pthread_t consumidores[consumidores_num];

    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    fclose(fileptr);
    int index[productores_num];
    for (int i = 0; i < productores_num; i++) {
        index[i] = i;
        if (pthread_create(&productores[i], NULL, read_file, &index[i]) != 0) {
            errorExit("Error, no se pudo crear thread");
        }
    }
    for (int i = 0; i < productores_num; i++) {
        if (pthread_join(productores[i], NULL) != 0) {
            errorExit("Error, no se pudo unir thread");
        }
    }

    // buffer = (char *) malloc(filelen * sizeof(char));
    // fread(buffer, filelen, 1, fileptr);
    // printf("%s\n", buffer);
    // fclose(fileptr);
    return 0;
}