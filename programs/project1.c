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

//buffer for reading
static unsigned char buffer[BUFFERLEN];

//array where things are going to get counted
static int solution_array[256];
static int solution_aux[256];

void merge(int arr1[], int arr2[], int left1[], int left2[], int left_size, int right1[], int right2[], int right_size) {
	int i = 0, j = 0, k = 0;
	while (i < left_size && j < right_size) {
		if (left1[i] <= right1[j]) {
			arr1[k] = left1[i];
			arr2[k] = left2[i];
			i++;
		}
		else {
			arr1[k] = right1[j];
			arr2[k] = right2[j];
			j++;
		}
		k++;
	}
	
	while (i < left_size) {
		arr1[k] = left1[i];
		arr2[k] = left2[i];
		i++;
		k++;
	}
	
	while (j < right_size) {
		arr1[k] = right1[j];
		arr2[k] = right2[j];
		j++;
		k++;
	}
}

void mergesort(int arr1[], int arr2[], int size) {
	if (size < 2)
		return;
	
	int mid = size / 2;
	int *left1 = (int*) malloc(mid * sizeof(int));
	int *left2 = (int*) malloc(mid * sizeof(int));
	int *right1 = (int*) malloc((size - mid) * sizeof(int));
	int *right2 = (int*) malloc((size - mid) * sizeof(int));
	
	for (int i = 0; i < mid; i++) {
		left1[i] = arr1[i];
		left2[i] = arr2[i];
	}
	
	for (int i = mid; i < size; i++) {
		right1[i - mid] = arr1[i];
		right2[i - mid] = arr2[i];
	}
	
	mergesort(left1, left2, mid);
	mergesort(right1, right2, size - mid);
	merge(arr1, arr2, left1, left2, mid, right1, right2, size - mid);
	
	free(left1);
	free(left2);
	free(right1);
	free(right2);
}

//inputs
static int producers_num = 0;
static int consumers_num = 0;

//file information
static char *filepath;
static long filelen;
static long bytes_consumers;

void *reading_file(void *thread_index) {
    int index = *((int *)thread_index);
    long reading_pos = index;
    FILE *fileptr;
    fileptr = fopen(filepath, "rb");
    while (!feof(fileptr)) {
        if (reading_pos <= filelen) fseek(fileptr, reading_pos, SEEK_SET);
        else {
            pthread_exit((void*)0);
        }
        pthread_mutex_lock(&buffer_mutex[reading_pos%BUFFERLEN]);
        printf("(1) Locked %ld\n", reading_pos);
        if (reading_pos >= BUFFERLEN) {
            while(read[reading_pos%BUFFERLEN]) {
                pthread_cond_wait(&consumed_condition[reading_pos%BUFFERLEN], &buffer_mutex[reading_pos%BUFFERLEN]);
            }
        }
        fread(buffer+(reading_pos%BUFFERLEN), 1, 1, fileptr);
        read[reading_pos%BUFFERLEN] = 1;
        printf("(1) Unlocked %ld\n", reading_pos);
        pthread_mutex_unlock(&buffer_mutex[reading_pos%BUFFERLEN]);
        pthread_cond_signal(&read_condition[reading_pos%BUFFERLEN]);
        reading_pos += producers_num;
    }
    fclose(fileptr);
    pthread_exit((void*)0);
}

void *adding_to_array(void *thread_index) {
    int index = *((int *)thread_index);
    long consuming_pos = index;
    while(bytes_consumers) {
        pthread_mutex_lock(&buffer_mutex[consuming_pos%BUFFERLEN]);
        printf("(2) Locked %ld\n", consuming_pos);
        if (!bytes_consumers || consuming_pos > filelen) {
            pthread_mutex_unlock(&buffer_mutex[consuming_pos%BUFFERLEN]);
            pthread_exit((void*)0);
        }
        while (!read[consuming_pos % BUFFERLEN] && buffer[consuming_pos % BUFFERLEN != '\000']) {
            pthread_cond_wait(&read_condition[consuming_pos%BUFFERLEN], &buffer_mutex[consuming_pos%BUFFERLEN]);
        }
        if (!bytes_consumers || consuming_pos > filelen) {
            pthread_mutex_unlock(&buffer_mutex[consuming_pos%BUFFERLEN]);
            pthread_exit((void*)0);
        }
        pthread_mutex_lock(&bytes_consumer_mutex);
        bytes_consumers--;
        pthread_mutex_unlock(&bytes_consumer_mutex);
        pthread_mutex_lock(&solution_mutex[consuming_pos%256]);
        solution_array[buffer[consuming_pos%BUFFERLEN]]++;
        buffer[consuming_pos%BUFFERLEN] = '\000';
        pthread_mutex_unlock(&solution_mutex[consuming_pos%256]);
        read[consuming_pos%BUFFERLEN] = 0;
        printf("(2) Unlocked %ld\n", consuming_pos);
        pthread_mutex_unlock(&buffer_mutex[consuming_pos%BUFFERLEN]);
        pthread_cond_signal(&consumed_condition[consuming_pos%BUFFERLEN]);
        consuming_pos += consumers_num;
    }
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < BUFFERLEN; i++) {
        if (i < 256) {
            pthread_mutex_init(&solution_mutex[i], NULL);
            solution_array[i] = 0;
            solution_aux[i] = i;
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
