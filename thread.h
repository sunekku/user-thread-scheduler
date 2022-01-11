#ifndef _THREAD_H_
#define _THREAD_H_

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
struct wait_queue *wait_queue_create();
void wait_queue_destroy(struct wait_queue *wq);
Tid thread_sleep(struct wait_queue *queue);
int thread_wakeup(struct wait_queue *queue, int all);
struct lock *lock_create();
void lock_destroy(struct lock *lock);
void lock_acquire(struct lock *lock);
void lock_release(struct lock *lock);
struct cv *cv_create();
void cv_destroy(struct cv *cv);
void cv_wait(struct cv *cv, struct lock *lock);
void cv_signal(struct cv *cv, struct lock *lock);
void cv_broadcast(struct cv *cv, struct lock *lock);

#endif