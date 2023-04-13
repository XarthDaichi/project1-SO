#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

static int productores_num = 0;
static int consumidores_num = 0;
static int array_bytes[256];


void errorExit(char *strerr) {
    perror(strerr);
    exit(1);
}

void *read_file(void *filelen, void *offset);

int main(int argc, char* argv[]) {
    char *filepath;
    if (argc > 1) {
        productores_num = atoi(argv[1]);
    } else errorExit("Argumento de entrada faltante, número de productores\n");

    if (argc > 2) {
        consumidores_num = atoi(argv[2]);
    } else errorExit("Argumento de entrada faltante, número de consumidores\n");

    if (argc > 3) {
        filepath = (argv[3]);
    } else errorExit("Argumento de entrada faltante, no hay archivo\n");
    
    pthread_t productores[productores_num];
    pthread_t consumidores[consumidores_num];

    FILE *fileptr;
    char *buffer;
    long filelen;
    fileptr = fopen(filepath, "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);

    buffer = (char *) malloc(filelen * sizeof(char));
    fread(buffer, filelen, filelen+3, fileptr);
    printf("%s\n", buffer);
    fclose(fileptr);
    return 0;
}