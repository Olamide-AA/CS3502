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

	int consumer_id = atoi(argv[1]);
	int num_items = atoi(argv[2]);

        int shm_id = -1;

	while (shm_id == -1) {
    		shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666);
	}

	shared_buffer_t *buffer = (shared_buffer_t *)shmat(shm_id, NULL, 0);
	if (buffer == (void *)-1) {
    		fprintf(stderr, "shmat failed\n");
		return 1;
	}


	while (buffer->ready == 0) {

	}

        // Open three named semaphores
        sem_t *sem_empty = sem_open("/sem_empty", O_CREAT, 0644, BUFFER_SIZE);
        sem_t *sem_full  = sem_open("/sem_full",  O_CREAT, 0644, 0);
        sem_t *sem_mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);

        if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED || sem_mutex == SEM_FAILED) {
                fprintf(stderr, "sem_open failed\n");
                return 1;
        }

	for (int i = 0; i < num_items; i++) {
    		sem_wait(sem_full);
    		sem_wait(sem_mutex);
		item_t item = buffer->buffer[buffer->tail];
		buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
		buffer->count--;

    		sem_post(sem_mutex);
    		sem_post(sem_empty);

		printf("Consumer %d: Consumed value %d from Producer %d\n",
    		consumer_id, item.value, item.producer_id);
	}

	sem_close(sem_empty);
	sem_close(sem_full);
	sem_close(sem_mutex);
	shmdt(buffer);
	

	return 0;
}
