#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#include "signals.h"
#include "built-in.h"
#include "user.h"
#include "processes.h"

extern int child_status, len_args;
extern pid_t child_pid;
extern char* args[];
extern struct Node* processes;
extern volatile sig_atomic_t fg_mode;

struct Node* create_node(pid_t pid) {
	// Returns a new node with pid and next initialized

	struct Node* new_node = malloc(sizeof(struct Node));
	new_node->pid = pid;
	new_node->next = NULL;
	return new_node;
}

void add_node(pid_t pid) {
	// Adds a new node to the end of a linked list
	struct Node* curr;

	// add as head if list is empty
	if (processes == NULL) {
		processes = create_node(pid);
	}
	else {

		curr = processes;
		while (curr->next != NULL)		// navigate to tail
			curr = curr->next;

		// create and add new node
		curr->next = create_node(pid);
	}
}

int ll_contains(pid_t pid) {
	// Returns 1 if the list contains the pid, 0 otherwise
	struct Node* curr = processes;

	while (curr != NULL) {
		if (curr->pid == pid)
			return 1;
		curr = curr->next;
	}

	return 0;
}

void remove_value(pid_t pid) {
	// Takes a pid and removes the matching node from the list
	struct Node* curr = processes, *prev;

	while (curr != NULL) {

		// Node is head
		if (curr == processes && curr->pid == pid) {
			processes = curr->next;
			free(curr);
			return;
		}

		// Node is within the list or is the tail
		else if (curr->pid == pid) {
			prev->next = curr->next;
			free(curr);
			return;
		}

		prev = curr;
		curr = curr->next;
	}
}

void reap_background() {
	// Closes recently completed background processes and prints PID and status

	child_pid = waitpid(-1, &child_status, WNOHANG);		// check for completed background processes
	if (ll_contains(child_pid)) {
		printf("background pid %d is done: ", child_pid);	// print PID and status
		fflush(stdout);
		get_status();
		remove_value(child_pid);			// remove from list of bg processes
	}
}

int is_background() {
	// Returns 1 if background process, 0 otherwise
	return !strcmp(args[len_args - 1], "&");
}

void redirect_in(char* in, char* path, bool bg) {
	// Redirect stdin from the given path, or /dev/null for background processes
	int sourceFD, result;
	char* bg_path;
	
	// Grab bg path if specified
	if (path == NULL)
		bg_path = "/dev/null";
	else
		bg_path = path;

	if (bg)
		sourceFD = open(bg_path, O_RDONLY);		// background process path
	else
		sourceFD = open(path, O_RDONLY);		// open the given path in read only mode

	// Error handler
	if (sourceFD == -1) {
		perror("source open()");
		free(in);			// free arguments because loop will discontinue
		free(path);
		free(args[0]);		// command
		exit(1);
	}

	// Redirect stdin to new source
	result = dup2(sourceFD, 0);
	if (result == -1) {
		perror("source dup2()");
		free(in);			// loop discontinues here as well
		free(path);
		free(args[0]);		// command
		exit(2);
	}
}

void redirect_out(char* out, char* path, bool bg) {
	// Redirect stdout to the given path, or /dev/null for background processes
	int targetFD, result;
	char* bg_path;

	// Grab bg path if specified
	if (path == NULL)
		bg_path = "/dev/null";
	else
		bg_path = path;

	// Ppen in write only, truncate mode; create ff it doesn't exist
	if (bg)					// background process path
		targetFD = open(bg_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);	// -rw-rw----
	else
		targetFD = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0640);		// -rw-rw----
	
	// Error handler
	if (targetFD == -1) {
		perror("target open()");
		free(out);			// free arguments because loop will discontinue
		free(path);
		free(args[0]);		// command
		exit(1);
	}

	// Redirect stdout to targetFD
	result = dup2(targetFD, 1);
	if (result == -1) {
		perror("target dup2()");
		free(out);			// loop discontinues here as well
		free(path);
		free(args[0]);		// command
		exit(2);
	}
}

void redirect(bool bg) {
	// Parses arguments for in or out and opens/redirects the specified path
	// Pre-condition: background trigger already removed
	int shift_count = 0;

	// Background processes redirect to /dev/null if unspecified
	if (bg) {
		for (int i = 0; i < len_args; i++) {
			if (strcmp(args[i], "<") == 0) {
				redirect_in(args[i], args[i + 1], bg);
				shift_count++;
			}
			else if (strcmp(args[i], ">") == 0) {
				redirect_out(args[i], args[i + 1], bg);
				shift_count++;
			}
		}
	}
	// Foreground processes redirect to a specified path
	else {
		for (int i = 0; i < len_args; i++) {
			if (strcmp(args[i], "<") == 0) {
				redirect_in(args[i], args[i + 1], bg);
				shift_count++;
			}
			else if (strcmp(args[i], ">") == 0) {
				redirect_out(args[i], args[i + 1], bg);
				shift_count++;
			}
		}
	}

	// Recalculate array length and dealloc arguments that were removed
	if (shift_count) {
		int new_len = len_args - (2 * shift_count);			// recalculate length
		for (int i = len_args - 1; i >= new_len; i--) {		// free args
			free(args[i]);
		}
		len_args = new_len;			// update length
		args[new_len] = NULL;
	}
}

void remove_bg_trigger() {
	// Removes background trigger and updates array length
	// Pre-condition: arguments for a background process only

	args[len_args - 1] = NULL;
	len_args--;
}

void manage_foreground() {
	// Manages foreground forks
	pid_t spawn_pid;

	// Remove background trigger if in foreground-only mode
	if (fg_mode && is_background())
		remove_bg_trigger();

	// Start child process
	spawn_pid = fork();
	switch (spawn_pid) {
	case -1:
		perror("fork() failed!\n");
		exit(1);
		break;
	case 0:
		reg_SIGINT();				// default SIGINT for foreground child
		redirect(false);			// check for redirects; bg = false
		execvp(args[0], args);		// execute the program

		// If execvp fails
		perror(args[0]);			// give the command
		free_args();				// free alloc'd memory
		exit(EXIT_FAILURE);			// exit the process
		break;
	default:
		reg_SIGTSTP();				// custom SIGTSTP handler for parents only
		waitpid(spawn_pid, &child_status, 0);

		// display status if child was terminated abnormally
		if (WIFSIGNALED(child_status)) {
			printf("terminated by signal %i\n", WTERMSIG(child_status));
			fflush(stdout);
		}
	}
}

void manage_background() {
	// Manages background forks
	pid_t spawn_pid;

	// Remove background trigger
	remove_bg_trigger();

	// Start child process
	spawn_pid = fork();
	switch (spawn_pid) {
	case -1:
		perror("fork() failed!\n");
		exit(1);
		break;
	case 0:
		redirect(true);						// check for redirects; bg = true
		execvp(args[0], args);				// execute the program

		// If execvp fails
		perror(args[0]);		// give the command
		free_args();			// free alloc'd memory
		exit(EXIT_FAILURE);		// exit the process
		break;	
	default:
		reg_SIGTSTP();			// custom SIGTSTP handler for parents only
		printf("background pid is %d\n", spawn_pid);
		fflush(stdout);
		add_node(spawn_pid);	// save the pid in the list
	}
}
