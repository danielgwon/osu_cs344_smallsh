#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#include "user.h"

extern char* args[];
extern int len_args;

void free_args() {
	// Clean up function to free alloc'd memory
	for (int i = 0; i < len_args; i++) {
		free(args[i]);
	}
}

void dolla_dolla() {
	// Parses each command/argument to expand instances of $$ to smallsh PID

	char* word;
	pid_t pid = getpid();
	char str_pid[7], new_string[2049];		// max pid length = 6, max chars in input = 2048
	bool found = false;
	int ns_ct = 0;

	// Convert pid to string
	sprintf(str_pid, "%d", pid);

	// For each command/argument
	for (int i = 0; i < len_args; i++) {
		word = args[i];

		// Scan each letter for $$
		for (int j = 0; j < strlen(word); j++) {

			// Append final $ if odd numbered consecutives
			if (word[j] == '$' && j == strlen(word) - 1) {
				new_string[ns_ct] = '$';
			}

			// Expansion found
			else if (word[j] == '$' && word[j + 1] == '$') {

				// found at beginning of word
				if (ns_ct == 0) {							
					strcpy(new_string, str_pid);			// copy pid and append NULL
					new_string[strlen(str_pid)] = '\0';
				}

				// found after letters
				else {
					strcat(new_string, str_pid);			// concat pid to existing letters
				}

				ns_ct += strlen(str_pid) - 1;				// next index = (num of copied ltrs + len pid)
				found = true;
				j++;										// skip to the next pair
			}

			// No expansions found, use original letter
			else {
				new_string[ns_ct] = word[j];
			}

			ns_ct++;		// track new word index
		}

		// Save new word
		if (found)
			args[i] = strdup(new_string);

		memset(new_string, '\0', ns_ct);	// clear buffer
		ns_ct = 0;							// reset count
		found = false;						// reset found
	}
}

int get_input(char* user_input) {
	// Displays command prompt, takes user input and parses it into command/arguments
	// returns True if the input was a blank line or comment, and False otherwise

	char* token, * save_ptr, * command;
	int i = 0;
	
	// Prompt and save input
	printf(": ");
	fflush(stdout);
	fgets(user_input, 2049, stdin);
	command = strdup(user_input);
	
	// Tokenize and save
	token = strtok_r(command, " \n", &save_ptr);

	// check for blank lines and comments - return immediately if found
	if (token == NULL) {	// blank
		args[i] = NULL;
		free(command);
		return true;
	}
	else if (strncmp(token, "#", 1) == 0) {	// comment
		free(command);
		return true;
	}

	// continue
	while (token != NULL) {
		args[i] = strdup(token);
		i++;
		token = strtok_r(NULL, " \n", &save_ptr);		// get next argument
	}

	args[i] = NULL;
	free(command);

	// Save number of arguments; do not count NULL
	len_args = i;

	// Check for $$ expansions
	dolla_dolla();

	return false;		
}
