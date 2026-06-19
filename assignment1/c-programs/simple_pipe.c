#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
	int pipefd[2];
	pid_t pid;
	char buffer[100];
	char *message = "Hello from parent!";

	int returnstatus = pipe(pipefd);	// TODO: Create pipe using pipe(pipefd)

	if(returnstatus == -1){// Check for errors (pipe returns -1 on failure)
		printf("Unable to create pipe\n");
		return 1;
	}

	pid = fork();	// TODO: Fork the process

	if (pid == 0) {
		// Child process
		close(pipefd[1]);				// TODO: Close the write end (child only reads)
		read(pipefd[0], buffer, sizeof(buffer));	// TODO: Read from pipe into buffer
		printf("Message from parent: %s\n" , buffer);	// TODO: Print the received message
		close(pipefd[0]);				// TODO: Close read end
	} else {
		// Parent process
		close(pipefd[0]);					// TODO: Close the read end (parent only writes)
		write(pipefd[1], message, strlen(message) + 1); // TODO: Write message to pipe
		close(pipefd[1]);				// TODO: Close write end
		wait(NULL);					// TODO: Wait for child to finish
	}

	return 0;
}


