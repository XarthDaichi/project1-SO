#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int productores_num = 0;
static int consumidores_num = 0;
static long filelen;
static char *buffer;
static pthread_t *consumidores;
static int array_bytes[256];


void errorExit(char *strerr) {
    perror(strerr);
    exit(1);
}

void *modify_array(void *modifying_byte) {
    char _modifying_byte = *((char*)modifying_byte);
    pthread_mutex_lock(&mutex);
    array_bytes[_modifying_byte]++;
    pthread_mutex_unlock(&mutex);
}

void *read_file(void *thread_index) {
    long section_start = (filelen/productores_num) * *((int *)thread_index);
    long section_end = *((int *)thread_index) != (productores_num-1) ? (filelen/productores_num) * (*((int *)thread_index) + 1) : filelen; 
    int index_used[consumidores_num];
    for (int i = 0; i < consumidores_num; i++) index_used[i] = 0;
    for (int i = section_start; i < section_end; i++) {
        for (int j = 0; j < consumidores_num; j++) {
            if (pthread_create(&consumidores[j], NULL, modify_array, &buffer[i]) == 0) {
                index_used[j] = 1;
                break;
            }
            if (j == consumidores_num -1) j = 0;
        }
    }
    for (int i = 0; i < consumidores_num; i++) {
        if (index_used[i]) pthread_join(consumidores[i], NULL);
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) productores_num = atoi(argv[1]);
    else errorExit("Argumento de entrada faltante, número de productores\n");

    if (argc > 2) consumidores_num = atoi(argv[2]);
    else errorExit("Argumento de entrada faltante, número de consumidores\n");

    char *filepath;
    if (argc > 3) filepath = (argv[3]);
    else errorExit("Argumento de entrada faltante, no hay archivo\n");
    
    pthread_t productores[productores_num];
    consumidores = (pthread_t *) malloc(consumidores_num * sizeof(pthread_t));

    // lectura de archivo
    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);
    buffer = (char *) malloc(filelen * sizeof(char));
    fread(buffer, filelen, 1, fileptr);
    fclose(fileptr);

    int index[productores_num];
    for (int i = 0; i < filelen; i++) printf("%c", buffer[i]);
    printf("\n");
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

    for (int i = 0; i < 256; i++) {
        if (array_bytes[i] != 0) {
            printf("%d aparece %d veces\n", i, array_bytes[i]);
        }
    }
    return 0;
}