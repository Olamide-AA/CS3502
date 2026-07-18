#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_ACCOUNTS 3
#define NUM_THREADS 5
#define TRANSACTIONS_PER_TELLER 10
#define INITIAL_BALANCE 1000.0
#define AMOUNT 100.0

typedef struct {
		int account_id;
		double balance;
		int transaction_count;
	} Account;

Account accounts[NUM_ACCOUNTS];

double intended_net[NUM_THREADS];

void* teller_thread(void* arg) {
	int teller_id = *(int*)arg;

	unsigned int seed = time(NULL) + teller_id;

	for(int i = 0; i < TRANSACTIONS_PER_TELLER; i++) {
		int acct = rand_r(&seed) % NUM_ACCOUNTS;
		int transaction_type = rand_r(&seed) % 2;

		if(transaction_type == 0){
			double temp = accounts[acct].balance;
			usleep(100);
			accounts[acct].balance = temp + AMOUNT;
			accounts[acct].transaction_count++;
			intended_net[teller_id] += AMOUNT;
			printf("Teller %d: Deposit  %.2f to account %d (txn %d)\n",teller_id, AMOUNT, acct, i);
		}else {
			double temp = accounts[acct].balance;
			usleep(100);
			accounts[acct].balance = temp - AMOUNT;
			accounts[acct].transaction_count++;
			intended_net[teller_id] -= AMOUNT;
			printf("Teller %d: Withdraw  %.2f from account %d (txn %d)\n",teller_id, AMOUNT, acct, i);
		}
	}
	return NULL;
}


int main(void){
	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS];
	struct timespec start, end;

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
	}
	
	printf("Initial total balance: %.2f\n\n", NUM_ACCOUNTS * INITIAL_BALANCE);

	clock_gettime(CLOCK_MONOTONIC, &start);


	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i;
		pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
	}

	for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    	}

    	clock_gettime(CLOCK_MONOTONIC, &end);
    	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	
	printf("\n--- Final Report ---\n");
	double actual_total = 0.0;
	int total_txns = 0;
	
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf("Account %d: balance %.2f, transactions %d\n", accounts[i].account_id, accounts[i].balance, accounts[i].transaction_count);
		actual_total += accounts[i].balance;
		total_txns += accounts[i].transaction_count;
	}
	
	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE;
	
	for (int i = 0; i < NUM_THREADS; i++) {
		expected_total += intended_net[i];
	}

	printf("\nExpected total balance: %.2f\n", expected_total);
	printf("Actual total balance:   %.2f\n", actual_total);
	
	if (expected_total != actual_total) {
		printf(">>> MISMATCH of %.2f - race condition lost updates! <<<\n",expected_total - actual_total);
	} else {
		printf("Totals match this run - run again, races are probabilistic.\n");
	}
	
	printf("Recorded transactions:  %d (expected %d)\n",total_txns, NUM_THREADS * TRANSACTIONS_PER_TELLER);
	printf("Elapsed time: %.4f seconds\n", elapsed);

	return 0;
}

