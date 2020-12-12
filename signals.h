#pragma once

// include guards for doubly-declared functions
#ifndef SIGNALS_HEADER
#define SIGNALS_HEADER

#include <signal.h>

void SIGTSTP_handler(int signo);
void ignore_signals();
void reg_SIGTSTP();
void reg_SIGINT();

#endif