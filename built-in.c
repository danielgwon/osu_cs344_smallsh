#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "processes.h"
#include "built-in.h"

extern char* args[];
extern int child_status;
extern struct Node* processes;

void change_dir() {
	// Change shell to a specific path or root if no arguments given

	char* file_path = args[1];

	// Root
	if (file_path == NULL)
		chdir(getenv("HOME"));

	// Path
	else
		chdir(file_path);

}

void processes_clean_up() {
	// Kills all remaining background processes

	struct Node* curr = processes, * prev = curr;

	// Search Linked List for processes
	while (curr != NULL) {
		kill(curr->pid, SIGTERM);	// kill and move to the next
		prev = curr;
		curr = curr->next;
		remove_value(prev->pid);	// remove from the list
	}
}

void get_status() {
	// Interpret and display the exit status of the last completed child process

	// Normal termination
	if (WIFEXITED(child_status)) {
		printf("exit value %i\n", WEXITSTATUS(child_status));
		fflush(stdout);
	}

	// Abnormal termination
	else {
		printf("terminated by signal %i\n", WTERMSIG(child_status));
		fflush(stdout);
	}
}