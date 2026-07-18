CS 3502 Project 1 - Multi-Threaded Banking System
Olamide Akintobi Adedeji

OVERVIEW

This project implements a multi-threaded banking system in C using POSIX
threads. It demonstrates race conditions, mutex synchronization, deadlock
creation, and deadlock resolution across four phases.

FILES

phase1.c  Basic threading. Five teller threads perform random deposits and
          withdrawals on three shared accounts with no synchronization.
          Each thread tracks the net change it intended to make, and the
          program compares the expected total against the actual total to
          show that unsynchronized read-modify-write updates get lost.
          A short sleep between the read and the write widens the race
          window so the incorrect results appear reliably.

phase2.c  Same workload as phase 1 with a pthread mutex added to each
          account. Every balance update happens inside a critical section,
          so the expected and actual totals always match. Timing code
          shows the performance cost of synchronization compared to
          phase 1.

phase3.c  Deadlock demonstration. Two threads transfer money between two
          accounts in opposite directions. Each transfer locks the source
          account, sleeps briefly, then requests the destination account.
          The opposite lock orders create a circular wait and the program
          deadlocks almost immediately. Because deadlocked threads can
          never be joined, the main thread uses a watchdog loop that
          monitors a progress counter and reports the deadlock after five
          seconds without progress, then exits.

phase4.c  Deadlock resolution using lock ordering. The scenario is
          identical to phase 3, but the transfer function always acquires
          the lower account ID first regardless of transfer direction.
          This invalidates the circular wait condition, so deadlock is
          impossible. The phase 3 watchdog is kept in place and never
          fires, and all transfers complete with correct final balances.

RESOLUTION STRATEGY CHOICE

I chose lock ordering because it prevents deadlock entirely rather than
detecting or recovering from it, it adds no runtime overhead, and it is
the standard approach for the fixed set of resources in this system.

COMPILATION

gcc -Wall -pthread phase1.c -o phase1
gcc -Wall -pthread phase2.c -o phase2
gcc -Wall -pthread phase3.c -o phase3
gcc -Wall -pthread phase4.c -o phase4

Run each phase with ./phase1, ./phase2, ./phase3, ./phase4.
Note that phase3 is expected to hang and will report the deadlock and
exit on its own after about five seconds.