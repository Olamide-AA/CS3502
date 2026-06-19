#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
	int pipefd1[2];
	int pipefd2[2];
	pid_t pid;
	char buffer[100];
	char *message1 = "Hello from parent!";
	char *message2 = "Hello from child!";

   	int returnstatus1 = pipe(pipefd1);
   	int returnstatus2 = pipe(pipefd2);

   	if(returnstatus1 == -1){
		printf("Unable to create pipe 1\n");
		return 1;
	}


   	if(returnstatus2 == -1){
		printf("Unable to create pipe 2\n");
		return 1;
	}

    	pid = fork();

    	if (pid == 0) {
        	// Child Process
        	close(pipefd1[1]);
		close(pipefd2[0]);

		read(pipefd1[0], buffer, sizeof(buffer));
		printf("Message from parent: %s\n" , buffer);

		write(pipefd2[1], message2, strlen(message2) + 1);

		close(pipefd1[0]);
		close(pipefd2[1]);
	} else {
		//Parent process
                close(pipefd1[0]);
                close(pipefd2[1]);

		write(pipefd1[1], message1, strlen(message1) + 1);

		read(pipefd2[0], buffer, sizeof(buffer));
		printf("Message from child: %s\n" , buffer);

 		close(pipefd1[1]);
                close(pipefd2[0]);

		wait(NULL);
    }

    return 0;
}
