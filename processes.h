#pragma once

// include guards for doubly-declared functions
#ifndef PROCESSES_HEADER
#define PROCESSES_HEADER

#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>

// Linked List
struct Node {
	pid_t pid;
	struct Node* next;
};
struct Node* create_node(pid_t pid);
void add_node(pid_t pid);
int ll_contains(pid_t pid);
void remove_value(pid_t pid);

// IO Redirects
void redirect_in(char* in, char* path, bool bg);
void redirect_out(char* out, char* path, bool bg);
void redirect(bool bg);

// Process management
void reap_background();
int is_background();
void manage_foreground();
void manage_background();

#endif