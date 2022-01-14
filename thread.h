#ifndef _THREAD_H_
#define _THREAD_H_

#include <ucontext.h>

typedef int Tid;
#define THREAD_MAX_THREADS 1024
#define THREAD_MIN_STACK 32768

enum { THREAD_ANY = -1,
	THREAD_SELF = -2,
	THREAD_INVALID = -3,
	THREAD_NONE = -4,
	THREAD_NOMORE = -5,
	THREAD_NOMEMORY = -6,
	THREAD_FAILED = -7
};

enum state{READY, RUNNING, BLOCKED};

typedef struct Thread{
    Tid tid;
    enum state t_state;
    void *sp;
    ucontext_t t_context;
}Thread;

typedef struct t_queue{
    Thread thread;
    struct t_queue *next;
}T_queue;

typedef struct w_queue{
    T_queue *f;
    T_queue *r;
}W_queue;

typedef struct Lock{
    int state;
}Lock;

typedef struct CV{
    W_queue *wq;
}CV;

static inline int
thread_ret_ok(Tid ret)
{
	return (ret >= 0 ? 1 : 0);
}

void thread_init(void);
Tid thread_id();
Tid thread_create(void (*fn) (void *), void *arg);
Tid thread_yield(Tid tid);
Tid thread_exit();
Tid thread_kill(Tid tid);
W_queue *wait_queue_create();
void wait_queue_destroy(W_queue *wq);
Tid thread_sleep(W_queue *queue);
int thread_wakeup(W_queue *queue, int all);
Lock *lock_create();
void lock_destroy(Lock *lock);
void lock_acquire(Lock *lock);
void lock_release(Lock *lock);
CV *cv_create();
void cv_destroy(CV *cv);
void cv_wait(CV *cv, Lock *lock);
void cv_signal(CV *cv, Lock *lock);
void cv_broadcast(CV *cv, Lock *lock);

#endif