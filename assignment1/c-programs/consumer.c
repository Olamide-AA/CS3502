#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t shutdown_flag = 0;
volatile sig_atomic_t print_stats_flag = 0;

void handle_sigint(int sig){
	shutdown_flag = 1;
}

void handle_sigusr1(int sig){
	print_stats_flag = 1;
}

int main(int argc, char *argv[]) {
	struct sigaction sa_int, sa_usr1;

	sa_int.sa_handler = handle_sigint;
	sigemptyset(&sa_int.sa_mask);
	sa_int.sa_flags = 0;
	sigaction(SIGINT, &sa_int, NULL);

	sa_usr1.sa_handler = handle_sigusr1;
	sigemptyset(&sa_usr1.sa_mask);
	sa_usr1.sa_flags = 0;
	sigaction(SIGUSR1, &sa_usr1, NULL);

	int max_lines = -1;
	int verbose = 0;
	int line_count = 0;
	int char_count = 0;
	int opt;

	while((opt = getopt(argc, argv, "n:v")) != -1) {
		switch (opt) {
			case 'n':
				max_lines = atoi(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				fprintf(stderr, "Usage: %s [-n line] [-v verbose]\n",  argv[0]);
				exit(1);
		}
	}

	char line[1024];

	while (fgets(line, sizeof(line), stdin) != NULL) {
		if (shutdown_flag) {
			break;
		}

        	line_count++;
        	char_count += strlen(line);

        	if (verbose) {
                	printf("%s", line);
        	}

		if (print_stats_flag) {
			fprintf(stderr, "[STATS] LInes so far: %d, Characters so far: %d\n", line_count, char_count);
			print_stats_flag = 0;
		}

        	if (max_lines > 0 && line_count >= max_lines) {
                	break;
        	}
	}

	fprintf(stderr, "Lines: %d, Characters: %d\n", line_count, char_count);
	return 0;
}
