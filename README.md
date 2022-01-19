# user-thread-scheduler

Primitive preemptive user-level thread framework that enables thread creation and deletion. 
Other utilities allow sychronization (condition variables) and mutual exclusion (mutex locks).
For Linux systems. Main thread must use thread_init() to initialize system. Other threads begin
execution from within a stub function and call thread_exit() from within as well.

Occasionally drew inspiration from existing frameworks for this one, but it was such a headache to figure out
that I might as well take all the credit. Nobody does anything new from scratch unless they're being
paid. 
