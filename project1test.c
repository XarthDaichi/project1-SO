#include <stdio.h>     //scanf/printf
#include <stdlib.h>    //conversiones
#include <unistd.h>    //fichero
#include <sys/stat.h>  //fichero
#include <sys/types.h>
#include <fcntl.h>    //fichero
#include <pthread.h>  //threads
#include <errno.h> // control de errores

// gcc -g -pthread project1.c -o project1

// void* routine() {
//     printf("test from threads\n");
//     sleep(3);
//     printf("Ending thread\n");
// }

// void* computation(void *add) {
//     long *add_num = (long*) (add);
//     printf("Add: %ld\n", *add_num);
// }

void* thread_routine(void* arg) {
    int nr_lines = *((int*)arg);
    int fd;
    char buf[] = "Nueva linea \n";

    printf("El hilo comienza a ejecutarse... \n");

    for (int i = 0; i < nr_lines; i++) {
        fd = open("./file.txt", O_WRONLY|O_APPEND);
        write(fd, buf, sizeof(buf)-1);
        close(fd);
    }
}

static int count = 10;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread1_routine(void *unused) {
    for (int i = 0; i < 10000; i++) {
        pthread_mutex_lock(&mutex);
        count++;
        pthread_mutex_unlock(&mutex);
    }
}

void *thread2_routine(void *unused) {
    for (int i = 0; i < 10000; i++) {
        pthread_mutex_lock(&mutex);
        count--;
        pthread_mutex_unlock(&mutex);
    }
}

void errorExit(char *strerr) {
    perror(strerr);
    exit(1);
}

int main(int argc, char* argv[]) {
    // pthread_t t1, t2, t3, t4;
    // int value = 0;
    // if (argc > 1) {
    //     value = atoi(argv[1]);
    // } else {
    //     printf("Argumento de entrada faltante, número de líneas por escribir (baboso)\n");
    // }
    // long value1 = 5;
    //pthread_create(pointer to thread, arguments for thread, pointer to function, arguments of function)
    // if(pthread_create(&t1, NULL, &routine, NULL) != 0) {
    //     return 1;
    // }
    // if (pthread_create(&t2, NULL, &routine, NULL) != 0) {
    //     return 2;
    // }
    // if (pthread_create(&t3, NULL, &computation, (void*) &value1) != 0) {
    //     return 3;
    // }
    // if (pthread_join(t1, NULL) != 0) {
    //     return 4;
    // }
    // if (pthread_join(t2, NULL) != 0) {
    //     return 5;
    // }
    // if (pthread_join(t3, NULL) != 0) {
    //     return 6;
    // }
    // if (pthread_create(&t4, NULL, thread_routine, &value) != 0) {
    //     return -1;
    // }
    // pthread_join(t4, NULL);

    pthread_t thread1, thread2;
    if (pthread_create(&thread1, NULL, thread1_routine, NULL) != 0)
        errorExit("thread1 cannot be created");
    if (pthread_create(&thread2, NULL, thread2_routine, NULL) != 0)
        errorExit("thread2 cannot be created");
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("count value %d \n", count);
    return 0;
}