#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "scheduler.h"

T_queue *dequeue(T_queue **qfrnt, T_queue **qrear, Tid tid){
    if(*qfrnt == NULL) return NULL;
    T_queue *curr = *qfrnt;
    T_queue *prev = NULL;
    while(curr){
        if((curr -> thread).tid == tid){
            if(!prev){
                if(curr == *qrear) *qrear = NULL;
                *qfrnt = curr -> next;
            }else{
                if(curr == *qrear) *qrear = prev;
                prev -> next = curr -> next;
            }
            return curr;
        }
        prev = curr;
        curr = curr -> next;
    }
    return NULL;
}

/*make sure the return is even necessary*/
Tid enqueue(T_queue **qfrnt, T_queue **qrear, T_queue *new){
    if(!(*qfrnt)){
        *qfrnt = new;
        *qrear = new;
        return (new -> thread).tid;
    }
    (*qrear) -> next = new;
    *qrear = new;
    return (new -> thread).tid;
}


Tid numT; //number of threads running
T_queue *t_curr;
T_queue t_curs;
T_queue *ready_f;
T_queue *ready_r;
T_queue *kill_f;
T_queue *kill_r;
Tid avail_id[THREAD_MAX_THREADS];
int numKT; //number of killed threads
 

void tkill(){
    while(kill_f){
        /*if(!(kill_f -> thread).tid){
            kill_f = kill_f -> next;
            continue;
        }*/
        T_queue *t = dequeue(&kill_f, &kill_r, (kill_f -> thread).tid);
        free((t -> thread).sp);
        avail_id[numKT] = (t -> thread).tid;
        numKT++;
        free(t);
        numT--;
    }
}

void thread_init(){
    numT = 1;
    t_curs.thread.tid = 0;
    t_curs.thread.t_state = RUNNING;
    t_curs.thread.sp = NULL; //is not on the heap
    t_curr = &t_curs;
    ready_f = NULL;
    ready_r = NULL;
    kill_f = NULL;
    kill_r = NULL;
    numKT = 0;
}

Tid thread_id(){
    return (t_curr -> thread).tid;
}

/*fix interrupts*/
void thread_stub(void(*fn)(void *), void *argv) {
    Tid ret;
    int enable = interrupts_set(1);
    fn(argv);
    interrupts_set(enable);
    ret = thread_exit();
    assert(ret == THREAD_NONE);
    exit(0);
}

Tid thread_create(void (*f)(void *), void *parg){
    int enable = interrupts_set(0); 
    if (numT >= THREAD_MAX_THREADS) {
	    interrupts_set(enable); 
	    return THREAD_NOMORE;
	}
    T_queue *new = (T_queue *)malloc(sizeof(T_queue));
    if(!new){
        interrupts_set(enable);
        return THREAD_NOMEMORY;
    }
    ucontext_t ncontext;
    int err = getcontext(&ncontext);
    assert(!err);
    ncontext.uc_stack.ss_sp = malloc(THREAD_MIN_STACK); //see if this even necessary
    if(!ncontext.uc_stack.ss_sp){
        free(new);
        interrupts_set(enable); 
        return THREAD_NOMEMORY;
    }
    ncontext.uc_stack.ss_size = THREAD_MIN_STACK;
    unsigned long int stack = (unsigned long int)ncontext.uc_stack.ss_sp + THREAD_MIN_STACK; //may need fix
    stack = (stack & -16L) - 8; //may need fix
    ncontext.uc_mcontext.gregs[REG_RSP] = stack;
    ncontext.uc_mcontext.gregs[REG_RIP] = (unsigned long int)thread_stub;
    ncontext.uc_mcontext.gregs[REG_RDI] = (unsigned long int)f;
    ncontext.uc_mcontext.gregs[REG_RSI] = (unsigned long int)parg;

    if(numKT == 0) (new -> thread).tid = numT++;
    else{
        (new -> thread).tid = avail_id[numKT - 1];
        numKT--;
        numT++;
    }
    (new -> thread).t_state = READY;
    (new -> thread).t_context = ncontext;
    (new -> thread).sp = ncontext.uc_stack.ss_sp;
    new -> next = NULL;
    enqueue(&ready_f, &ready_r, new);
    interrupts_set(enable); 
    return (new -> thread).tid;
}

/*fix interrupts*/
Tid thread_yield(Tid want_tid){
    int enable = interrupts_set(0); 
    tkill();
    volatile int swp = 0; 
    if(want_tid == THREAD_ANY){
        if(!ready_f){
            interrupts_set(enable);
            return THREAD_NONE;
        }else want_tid = (ready_f -> thread).tid;
    }
    if(want_tid == THREAD_SELF || want_tid == thread_id()){
        interrupts_set(enable); 
        return thread_id();
    }
    T_queue *t_run = dequeue(&ready_f, &ready_f, want_tid);
    if(!t_run){
        interrupts_set(enable); 
        return THREAD_INVALID;
    }
    //t_run -> next = NULL; 
    ucontext_t curr;
    int err = getcontext(&curr);
    assert(!err);
    if(!swp){
        swp = 1;
        (t_curr -> thread).t_context = curr;
        (t_curr -> thread).t_state = READY;
        t_curr -> next = NULL;
        enqueue(&ready_f, &ready_r, t_curr);
        (t_run -> thread).t_state = RUNNING;
        t_curr = t_run;

        setcontext(&((t_curr -> thread).t_context));
    }
    interrupts_set(enable);
    return want_tid;
}

/* check return */
Tid thread_exit(){
    int enable = interrupts_set(0); 
    tkill();
    if(!ready_f){
        interrupts_set(enable);
        return THREAD_NONE;
    }
    T_queue *t_run = dequeue(&ready_f, &ready_r, (ready_f -> thread).tid);
    Tid cid = thread_id();
    //t_run -> next = NULL;
    ucontext_t curr;
    int err = getcontext(&curr);
    assert(!err);
    (t_curr -> thread).t_context = curr;
    (t_run -> thread).t_state = RUNNING;
    enqueue(&kill_f, &kill_r, t_curr);
    t_curr = t_run;
    setcontext(&((t_curr -> thread).t_context));
    interrupts_set(enable);
    return cid;
}

Tid thread_kill(Tid tid){
    int enable = interrupts_set(0);
    tkill();
    T_queue *t_kill = dequeue(&ready_f, &ready_r, tid);
    if(!t_kill) return THREAD_INVALID;
    free((t_kill -> thread).sp);
    avail_id[numKT] = (t_kill -> thread).tid;
    numKT++;
    free(t_kill);
    numT--;
    interrupts_set(enable);
    return tid;
}

W_queue *wait_queue_create(){
    W_queue *q = malloc(sizeof(W_queue));
    assert(q);
    q -> f = NULL;
    q -> r = NULL;
}

void wait_queue_destroy(W_queue *q){
    free(q);
}

Tid thread_sleep(W_queue *q){
    int enable = interrupts_set(0);
    volatile int swp = 0;
    if(!q){
        interrupts_set(enable);
        return THREAD_INVALID;
    }
    if(!ready_f){
        interrupts_set(enable);
        return THREAD_NONE;
    }
    T_queue *t_run = dequeue(&ready_f, &ready_r, (ready_f -> thread).tid);
    ucontext_t curr;
    int err = getcontext(&curr);
    if(!swp){
        swp = 1;
        (t_curr -> thread).t_context = curr;
        (t_curr -> thread).t_state = BLOCKED;
        (t_run -> thread).t_state = RUNNING;
        enqueue(&(q -> f), &(q -> r), t_curr);
        t_curr = t_run;
        err = setcontext(&((t_curr -> thread).t_context));
        assert(!err);
    }
    interrupts_set(enable);
    return (t_curr -> thread).tid;
}

int thread_wakeup(W_queue *q, int all){
    int enable = interrupts_set(0);
    if(!q || !(q -> f)){
        interrupts_set(enable);
        return 0;
    }
    if(!all){
        T_queue *t_run = dequeue(&(q -> f), &(q -> r), (q -> f -> thread).tid);
        t_run -> next = NULL;
        (t_run -> thread).t_state = READY;
        enqueue(&ready_f, &ready_r, t_run);
        interrupts_set(enable);
        return 1;
    }else{
        int i = 0;
        while(q -> f){
            T_queue *t_run = dequeue(&(q -> f), &(q -> r), (q -> f -> thread).tid);
            t_run -> next = NULL;
            (t_run -> thread).t_state = READY;
            enqueue(&ready_f, &ready_r, t_run);
            i++;
        }
        interrupts_set(enable);
        return i;
    }
}

W_queue *queue;

int tset(Lock *lock){
    int enable = interrupts_set(0);
    int prev = lock -> state;
    lock -> state = 1;
    interrupts_set(enable);
    return prev;
}

Lock *lock_create(){
    Lock *lock = malloc(sizeof(Lock));
    assert(lock);
    lock -> state = 0;
    if(!queue) queue = wait_queue_create();
    return lock;
}

void lock_destroy(Lock *lock){
    assert(lock);
    assert(!(lock -> state));
    free(lock);
}

void lock_acquire(Lock *lock){
    assert(lock);
    lock -> state = 0;
    thread_wakeup(queue, 0);
}

/*make sure atomic instruction not needed*/
void lock_release(Lock *lock){
    assert(lock);
    lock -> state = 0;
    thread_wakeup(queue, 0);
}

CV *cv_create(){
    CV *cv = malloc(sizeof(CV));
    assert(cv);
    cv -> wq = wait_queue_create();
    return cv;
}

void cv_destroy(CV *cv){
    assert(cv);
    assert(!(cv -> wq -> f));
    wait_queue_destroy(cv -> wq);
    free(cv);
}

void cv_wait(CV *cv, Lock *lock){
    int enable = interrupts_set(0);
    assert(cv);
    assert(lock);
    assert(lock -> state);
    lock_release(lock);
    thread_sleep(cv -> wq);
    lock_acquire(lock);
    interrupts_set(enable);
}

void cv_signal(CV *cv, Lock *lock){
    int enable = interrupts_set(0);
    assert(cv);
    assert(lock);
    assert(lock -> state);
    thread_wakeup(cv -> wq, 0);
    lock -> state = 0;
    interrupts_set(enable);
}

void cv_broadcast(CV *cv, Lock *lock){
    int enable = interrupts_set(0);
    assert(cv);
    assert(lock);
    assert(lock -> state);
    thread_wakeup(cv -> wq, 1);
    lock -> state = 0;
    interrupts_set(enable);
}