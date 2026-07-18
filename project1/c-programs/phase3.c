#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS 2
#define TRANSFERS_PER_THREAD 100
#define INITIAL_BALANCE 1000.0
#define AMOUNT 50.0
#define WATCHDOG_TIMEOUT 5   

typedef struct {
		int account_id;
		double balance;
		int transaction_count;
		pthread_mutex_t lock;
	} Account;

Account accounts[NUM_ACCOUNTS];

double intended_net[NUM_THREADS];

volatile int completed_transfers = 0;
volatile int threads_done = 0;

typedef struct {
	int thread_id;
	int from;
	int to;
} TransferJob;

void transfer(int thread_id, int from_id, int to_id, double amount) {
	printf("Thread %d: locking account %d (source)\n", thread_id, from_id);
	pthread_mutex_lock(&accounts[from_id].lock);
	usleep(100);
	printf("Thread %d: holding %d, waiting for account %d\n",thread_id, from_id, to_id);
	pthread_mutex_lock(&accounts[to_id].lock);
	accounts[from_id].balance -= amount;
	accounts[to_id].balance += amount;
	accounts[from_id].transaction_count++;
	accounts[to_id].transaction_count++;

	pthread_mutex_unlock(&accounts[to_id].lock);
	pthread_mutex_unlock(&accounts[from_id].lock);

	completed_transfers++;
}

void* transfer_thread(void* arg) {
	TransferJob* job = (TransferJob*)arg;
	for (int i = 0; i < TRANSFERS_PER_THREAD; i++) {
	transfer(job->thread_id, job->from, job->to, AMOUNT);
	}
	printf("Thread %d: finished all transfers\n", job->thread_id);
	threads_done++;
	return NULL;
}

int main(void){
	pthread_t threads[NUM_THREADS];
	TransferJob jobs[NUM_THREADS];

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
	        pthread_mutex_init(&accounts[i].lock, NULL);
	}
	
	jobs[0] = (TransferJob){0, 0, 1};
	jobs[1] = (TransferJob){1, 1, 0};

	printf("Starting %d threads doing opposing transfers...\n\n", NUM_THREADS);
	
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, transfer_thread, &jobs[i]);
	}

	int last_progress = -1;
	int stalled_seconds = 0;

	while (threads_done < NUM_THREADS) {
		sleep(1);
		if (completed_transfers == last_progress) {
			stalled_seconds++;
			printf("[watchdog] no progress for %d s " "(stuck at %d completed transfers)\n",stalled_seconds, completed_transfers);
			if (stalled_seconds >= WATCHDOG_TIMEOUT) {
				printf("\n>>> DEADLOCK DETECTED <<<\n");
				printf("Threads made no progress for %d seconds.\n",
				       WATCHDOG_TIMEOUT);
				printf("Thread 0 holds account 0, waits for account 1.\n");
				printf("Thread 1 holds account 1, waits for account 0.\n");
				printf("Circular wait -> all four Coffman conditions met.\n");
				printf("Completed %d of %d transfers before deadlock.\n",completed_transfers,
	       			NUM_THREADS * TRANSFERS_PER_THREAD);
				exit(1);
		} else {
			last_progress = completed_transfers;
			stalled_seconds = 0;
		}
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		pthread_mutex_destroy(&accounts[i].lock);
	}

	for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    	}

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
        	pthread_mutex_destroy(&accounts[i].lock);
    	}

	printf("\nNo deadlock this run. Run again.\n");
	printf("Account 0: %.2f  Account 1: %.2f\n",
	accounts[0].balance, accounts[1].balance);
	return 0;
}

