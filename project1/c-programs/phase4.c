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

volatile int completed_transfers = 0;
volatile int threads_done = 0;

typedef struct {
	int thread_id;
	int from;
	int to;
} TransferJob;

void safe_transfer(int thread_id, int from_id, int to_id, double amount) {
	int first  = (from_id < to_id) ? from_id : to_id;
	int second = (from_id < to_id) ? to_id : from_id;

	pthread_mutex_lock(&accounts[first].lock);

	usleep(100);

	pthread_mutex_lock(&accounts[second].lock);

	accounts[from_id].balance -= amount;
	accounts[to_id].balance += amount;
	accounts[from_id].transaction_count++;
	accounts[to_id].transaction_count++;
	
	pthread_mutex_unlock(&accounts[second].lock);
	pthread_mutex_unlock(&accounts[first].lock);


	completed_transfers++;
}

void* transfer_thread(void* arg) {
	TransferJob* job = (TransferJob*)arg;
	for (int i = 0; i < TRANSFERS_PER_THREAD; i++) {
	safe_transfer(job->thread_id, job->from, job->to, AMOUNT);
	}
	printf("Thread %d: finished all %d transfers\n", job->thread_id, TRANSFERS_PER_THREAD);
	threads_done++;
	return NULL;
}

int main(void){
	pthread_t threads[NUM_THREADS];
	TransferJob jobs[NUM_THREADS];
	struct timespec start, end;

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
	        pthread_mutex_init(&accounts[i].lock, NULL);
	}
	
	jobs[0] = (TransferJob){0, 0, 1};
	jobs[1] = (TransferJob){1, 1, 0};

	printf("Starting %d threads doing opposing transfers...WITH lock ordering\n\n", NUM_THREADS);

	clock_gettime(CLOCK_MONOTONIC, &start);
	
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, transfer_thread, &jobs[i]);
	}

	int last_progress = -1;
	int stalled_seconds = 0;

	while (threads_done < NUM_THREADS) {
		sleep(1);
		if (completed_transfers == last_progress) {
			stalled_seconds++;
			printf("[watchdog] no progress for %d s\n ",stalled_seconds);
			if (stalled_seconds >= WATCHDOG_TIMEOUT) {
				printf("\n>>> DEADLOCK DETECTED <<<\n");
				exit(1);
			}
		} else {
			last_progress = completed_transfers;
			stalled_seconds = 0;
		}
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	clock_gettime(CLOCK_MONOTONIC, &end);

	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;


	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		pthread_mutex_destroy(&accounts[i].lock);
	}


	printf("\n--- Final Report ---\n");
	printf("Completed transfers: %d of %d\n", completed_transfers, NUM_THREADS * TRANSFERS_PER_THREAD);
	printf("Account 0: balance %.2f, transactions %d\n", accounts[0].balance, accounts[0].transaction_count);
	printf("Account 1: balance %.2f, transactions %d\n", accounts[1].balance, accounts[1].transaction_count);
	printf("Total balance: %.2f (should equal initial %.2f)\n",accounts[0].balance + accounts[1].balance, NUM_ACCOUNTS * INITIAL_BALANCE);
	printf("Elapsed time: %.4f seconds\n", elapsed);
	printf("\nNo deadlock: lock ordering broke the circular wait.\n");
	return 0;
}

