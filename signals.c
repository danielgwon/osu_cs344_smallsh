#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

#include "signals.h"

extern int child_status;
extern char* args[];
extern int len_args;
extern volatile sig_atomic_t fg_mode;

void SIGTSTP_handler(int signo) {
	// Allows user to toggle foreground-only mode in shell using SIGTSTP or Ctrl-z

	char* enter_msg = "\nEntering foreground-only mode (& is now ignored)\n";	// 50 chars
	char* exit_msg = "\nExiting foreground-only mode\n";						// 29 chars

	// Enter foreground mode
	if (fg_mode) {
		write(STDOUT_FILENO, exit_msg, 30);		// reentrant functions to avoid corrupting
		fg_mode = 0;							// normal function execution
	}

	// Exit foreground mode
	else {
		write(STDOUT_FILENO, enter_msg, 50);
		fg_mode = 1;
	}
}

void ignore_signals() {
	// Register handlers to ignore SIGINT and SIGTSTP

	struct sigaction ignore_action = { 0 };			// initialize sigaction
	ignore_action.sa_handler = SIG_IGN;				// set handler to ignore

	// no mask, flags, or alternative handlers

	// Register the signals
	sigaction(SIGINT, &ignore_action, NULL);
	sigaction(SIGTSTP, &ignore_action, NULL);
}

void reg_SIGTSTP() {
	// Register a custom SIGTSTP handler

	struct sigaction SIGTSTP_action = { 0 };		// initialize
	SIGTSTP_action.sa_handler = SIGTSTP_handler;	// set handler to custom
	sigfillset(&SIGTSTP_action.sa_mask);			// block all catchable
	SIGTSTP_action.sa_flags = SA_RESTART;			// restart interrupted system calls
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);		// register
}

void reg_SIGINT() {
	// Register the default handler for SIGINT

	struct sigaction SIGINT_action = {0};			// initialize
	SIGINT_action.sa_handler = SIG_DFL;				// set handler to default
	sigaction(SIGINT, &SIGINT_action, NULL);		// register
}
