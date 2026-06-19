#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t shutdown_flag = 0;

void handle_sigint(int sig) {
	shutdown_flag = 1;
}

int main(int argc, char*argv[]) {
	struct sigaction sa_int;
	sa_int.sa_handler = handle_sigint;
	sigemptyset(&sa_int.sa_mask);
	sa_int.sa_flags = 0;
	sigaction(SIGINT, &sa_int, NULL);

	char *filename = NULL;
	int buffer_size = 4096;
	int opt;

	while ((opt = getopt(argc, argv, "f:b:")) != -1) {
		switch (opt) {
			case 'f':
				filename = optarg; // optarg contains the argument value
				break;
			case 'b':
				buffer_size = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [-f file] [-b size]\n", argv[0]);
				exit(1);
		}
	}


	FILE *input = stdin;  // default
	if (filename!= NULL) {
    		input = fopen(filename, "r");
		if(input == NULL) {
			fprintf(stderr, "Error: could not open file %s\n", filename);
			exit(1);
		}
	}

	char *buf = malloc(buffer_size);
	size_t bytes_read;

	while ((bytes_read = fread(buf, 1, buffer_size, input)) > 0) {
		if (shutdown_flag) {
			break;
		}
    		fwrite(buf, 1, bytes_read, stdout);
	} 
	return 0;
}
