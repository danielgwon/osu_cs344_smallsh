#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "built-in.h"
#include "user.h"
#include "processes.h"
#include "signals.h"

int child_status, len_args = 0;
char* args[513];					// max 512 arguments
struct Node* processes = NULL;
volatile sig_atomic_t fg_mode = 0;
pid_t child_pid;

int main(int argc, char* argv[]) {
	if (argc >= 2) {
		printf("Incorrect usage. Please enter executable filename with no arguments\n");
		fflush(stdout);
	}

	char user_input[2049] = { 0 };	// max 2048 characters
	char* command;
	bool comment = false;

	while (true) {

		// Signal Handlers
		ignore_signals();				// ignore SIGINT and SIGTSTP as a base response

		comment = get_input(user_input);
		if (comment) {
			reap_background();
			continue;					// skip to get input
		}
		command = args[0];

		// Built-in Commands
		if (strcmp(command, "cd") == 0) {
			change_dir();
		}
		else if (strcmp(command, "exit") == 0) {
			processes_clean_up();		// close all background processes
			free_args();				// free alloc'd memory
			return 0;
		}
		else if (strcmp(command, "status") == 0) {
			get_status();
		}

		// System Commands
		else {
			if (is_background() && fg_mode == 0)		// skip if in fg mode
				manage_background();
			else
				manage_foreground();
		}

		reap_background();			// check for completed bg processes
		comment = false;
		free_args();
	}

	return 0;
}