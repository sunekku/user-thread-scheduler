#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <stdio.h>
#include <signal.h>

#define SIG_TYPE SIGALRM
#define SIG_INTERVAL 200

void register_interrupt_handler(int verbose);
int interrupts_on(void);
int interrupts_off(void);
int interrupts_set(int enabled);
int interrupts_enabled();
void interrupts_quiet();

void spin(int msecs);
int unintr_printf(const char *fmt, ...);
#endif