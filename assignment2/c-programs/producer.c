#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "../buffer.h"

int main(int argc, char *argv[]){
	if (argc != 3) {
    		fprintf(stderr, "Usage: %s <id> <num_items>\n", argv[0]);
    		return 1;
	}

	int producer_id = atoi(argv[1]);
	int num_items = atoi(argv[2]);

	// Create or get existing shared memory segment
	int shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666);

	if(shm_id == -1){
		printf("shmget failed\n");
		return 1;
	}


	// Attach to your process’s address space
	shared_buffer_t *buffer = (shared_buffer_t *)shmat(shm_id, NULL, 0);
	if (buffer == (void *)-1) {
    		fprintf(stderr, "shmat failed\n");
    		return 1;
	}

	// Initialize buffer on first use
	buffer->head = 0;
	buffer->tail = 0;
	buffer->count = 0;
	buffer->ready = 0;

	// Open three named semaphores
	sem_t *sem_empty = sem_open("/sem_empty", O_CREAT, 0644, BUFFER_SIZE);
	sem_t *sem_full  = sem_open("/sem_full",  O_CREAT, 0644, 0);
	sem_t *sem_mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);

	if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED || sem_mutex == SEM_FAILED) {
    		fprintf(stderr, "sem_open failed\n");
    		return 1;
	}

	buffer->ready = 1;

	for (int i = 0; i < num_items; i++) {
		item_t item;
		item.value = producer_id * 1000 + i;
		item.producer_id = producer_id;

		sem_wait(sem_empty);
    		sem_wait(sem_mutex);


    		buffer->buffer[buffer->head] = item;
    		buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    		buffer->count++;

    		sem_post(sem_mutex);
    		sem_post(sem_full);

    		printf("Producer %d: Produced value %d\n", producer_id, item.value);
	}

	shmdt(buffer);
	sem_close(sem_empty);
	sem_close(sem_full);
	sem_close(sem_mutex);

	return 0;
}
