#include <bits/types/timer_t.h>
#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <list>
#include <vector>
#include <iostream>
#include "thread.h"
#include <setjmp.h>
#include <unistd.h>
#include <algorithm>
#include "sleeping_threads_list.h"

#define SLEEP 3

//============================ Error Messages ============================//

#define SYS_ERR "system error: "
#define LIB_ERR "thread library error: "

#define MAX_TRD_ERR "Max number of threads reached.\n"


//============================ Globals ============================//
using namespace std;
// Total number of quantums started:
int total_quantum_num = 1;
// The id of the currently running thread:
int running_tid = 0;
// The total number of active threads:
int num_of_threads = 0;

// Signal set for blocked signals:
sigset_t blocked_signals;


struct sigaction sa;
// The virtual timer that handles quantums:
struct itimerval vtimer;

struct sigaction rsa;
// The real timer that handles sleeping threads:
struct itimerval rtimer;

// Queue for all ready threads:
list<int> ready;
// Queue for blocked threads:
list<int> blocked;
// Queue for sleeping threads:
list<int> sleeping;

// Vector of all threads
vector<Thread *> threads;

// List of sleeping threads to wake up.
SleepingThreadsList to_wakeup;

//============================ Function Declaration ============================//

void runThread();
void wake_thread(int sig);

//============================ Helper Functions ============================//

/*
 * Block signals
 * */
void block_signals()
{
//    sigaddset(&blocked_signals, SIGVTALRM);
    sigprocmask(SIG_BLOCK, &blocked_signals, NULL);
}


/*
 * Unblock signals
 * */
void unblock_signals()
{
    sigprocmask(SIG_UNBLOCK, &blocked_signals, NULL);
}


/*
 * Resets the alarm of the real timer of the sleeping threads.
 *
 * If no threads are sleeping, no alarm will be activated.
 * */
void reset_alarm()
{
    block_signals();
    if (to_wakeup.peek() != nullptr)
    {
        struct timeval curr_time;
        gettimeofday(&curr_time, nullptr);
        timersub(&to_wakeup.peek()->awaken_tv, &curr_time, &rtimer.it_value);
    } else
    {
        //Stop the timer:
        rtimer.it_value.tv_sec = 0;
        rtimer.it_value.tv_usec = 0;
    }
    // Start a real timer. It counts down in real time.
    if (setitimer(ITIMER_REAL, &rtimer, NULL))
    {
        printf("setitimer error. real timer\n");
    }
    unblock_signals();
}

/*
 * Description: Returns the first empty id for a thread. If no thread is empty, returns -1.
*/
int notValidTid(int tid)
{
    if (tid < 0 || tid >= MAX_THREAD_NUM || threads.at(tid) == nullptr)
    {
        if (tid < 0)
        {
            cerr << LIB_ERR
                 << "The thread id is invalid (it needs to be between 0 and " << MAX_THREAD_NUM - 1 << " ).\n";
        } else
        {
            cerr << LIB_ERR << "No thread with id " << tid << ".\n";

        }
        return -1;
    }
    return 0;
}

/*
 * Return the first available id for a thread.
 * */
int getFirstID()
{
    for (int i = 1; i < MAX_THREAD_NUM; ++i)
    {
        if (threads.at(i) == nullptr)
        {
            return i;
        }
    }
    cerr << LIB_ERR
         << MAX_TRD_ERR;
    return -1;
}


/*
 * Switches the running thread to the given state, and runs the next ready thread.
 * */
void switch_threads(int to_state)
{
    block_signals();

    if (threads[running_tid] != nullptr)
    {
        // Save the current context of the thread
        int ret_val = sigsetjmp(*threads[running_tid]->getEnv(), 1);
        if (ret_val != 0)
        { // If returning from another process, exit and continue the run normally
            unblock_signals();
            return;
        }

        threads[running_tid]->setState(to_state);
        if (to_state == READY)
        {
            // Push the currently running thread to the end of the ready queue
            ready.push_back(running_tid);
        } else if (to_state == BLOCKED)
        {
            // Push the currently running thread to the end of the blocked queue
            blocked.push_back(running_tid);
        } else
        {
            // Sleep:
            sleeping.push_back(running_tid);
            // Set the state of the thread to ready, to signal that it isn't blocked, so when waking up,
            // will be returned to ready queue
            threads[running_tid]->setState(READY);
        }
        runThread();
    }
    unblock_signals();

}

/*
 * Runs the next thread in the ready queue.
 * */
void runThread()
{
    // Update the total number of quantums and the quantums run by the specific thread.
    total_quantum_num++;
    // Set the next running thread to be the first in the ready queue
    running_tid = ready.front();
    ready.pop_front();
    threads[running_tid]->setState(RUNNING);
    unblock_signals();
    // Start running the next process:
    siglongjmp(*threads[running_tid]->getEnv(), 1);
}

/*
 * Handler for the virtual timers Signal - SIGVTALRM
 * */
void timer_handler(int sig)
{
    switch_threads(READY);
}

/*
 * Handler for the real timers Signal - SIGALRM
 * The function wakes up all sleeping threads whose time to wake up has passed.
 * */
void wake_thread(int sig)
{
    block_signals();
    do
    {
        int tid = to_wakeup.peek()->id;
        to_wakeup.pop();
        sleeping.remove(tid);
        // Case: Thread wasn't blocked while sleeping:
        if (threads[tid]->getState() == READY)
        {
            ready.push_back(tid);
        }
        struct timeval curr_time;
        gettimeofday(&curr_time, nullptr);
        if (to_wakeup.peek() != nullptr)
        {
            timersub(&to_wakeup.peek()->awaken_tv, &curr_time, &rtimer.it_value);
        }
    } while (to_wakeup.peek() != nullptr && (rtimer.it_value.tv_sec <= 0 || rtimer.it_value.tv_usec <= 0));
    reset_alarm();
    unblock_signals();
}




//============================ Library Functions ============================//
/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    if (quantum_usecs <= 0)
    {
        cerr << LIB_ERR
             << "quantum_usecs must be positive\n";
        return -1;
    }
    threads.resize(MAX_THREAD_NUM);
    threads.at(0) = new Thread(0, STACK_SIZE, nullptr);
    threads[0]->setState(RUNNING);

    //Initialize the thread of blocked signals.
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGVTALRM);
    sigaddset(&blocked_signals, SIGALRM);

    to_wakeup = SleepingThreadsList();

    // Set Signal functions:
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        printf("sigaction error.");
    }

    rsa.sa_handler = &wake_thread;
    if (sigaction(SIGALRM, &rsa, NULL) < 0)
    {
        printf("sigaction error.");
    }


    //
    // first time interval, seconds part
    vtimer.it_value.tv_sec = quantum_usecs / 1000000;
    // first time interval, microseconds part
    vtimer.it_value.tv_usec = quantum_usecs % 1000000;
    // following time intervals, seconds part
    vtimer.it_interval.tv_sec = quantum_usecs / 1000000;
    // following time intervals, microseconds part
    vtimer.it_interval.tv_usec = quantum_usecs % 1000000;

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &vtimer, NULL))
    {
        printf("setitimer error. virtual timer");
    }
    return 0;
}

/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
    block_signals();
    if (num_of_threads + 1 >= MAX_THREAD_NUM)
    {
        cerr << LIB_ERR << MAX_TRD_ERR;
        unblock_signals();
        return -1;
    }

    int newtid = getFirstID();
    threads[newtid] = new Thread(newtid, STACK_SIZE, f);
    ready.push_back(newtid);
    num_of_threads++;

    unblock_signals();

    return newtid;

}

/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
    block_signals();
    //Case: main suicide
    if (tid == 0)
    {
        // Clear all lists.
        ready.clear();
        blocked.clear();
        sleeping.clear();
        // Free memory of each thread.
        for (int remove = 0; remove < threads.size(); ++remove)
        {
            Thread *toDelete = threads[remove];
            to_wakeup.remove_thread(remove);
            if (toDelete != nullptr)
            {
                delete (toDelete);
                threads[remove] = nullptr;
            }
        }
        threads.clear();
        exit(0);
    }

    if (notValidTid(tid))
    {
        unblock_signals();
        return -1;
    }

    //Free memory of thread
    Thread *toDelete = threads.at(tid);
    delete (toDelete);
    threads[tid] = nullptr;
    num_of_threads--;

    //Remove from all threads list:
    blocked.remove(tid);
    ready.remove(tid);
    sleeping.remove(tid);
    //Remove the thread from the queue of threads to wake up
    to_wakeup.remove_thread(tid);
    //start the clock once again
    reset_alarm();

    //Case: Terminated self
    if (running_tid == tid)
    {
        runThread();
    }
    unblock_signals();
    return 0;
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
    block_signals();
    if (tid == 0)
    {
        cerr << LIB_ERR << "it's illegal to block the main thread\n";
        unblock_signals();
        return -1;
    }
    if (notValidTid(tid))
    {
        unblock_signals();
        return -1;
    }
    //Case: Thread is running:
    if (tid == running_tid)
    {
        if (setitimer(ITIMER_VIRTUAL, &vtimer, NULL))
        {
            printf("setitimer error.");
        }
        switch_threads(BLOCKED);
        return 0;
    }
    //Case: Thread is ready:
    if (threads[tid]->getState() == READY)
    {
        ready.remove(tid);
        blocked.push_back(tid);
        threads[tid]->setState(BLOCKED);
    }
    unblock_signals();
    return 0;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
    block_signals();
    if (notValidTid(tid))
    {
        unblock_signals();
        return -1;
    }

    //Case: Thread is blocked:
    if (threads[tid]->getState() == BLOCKED)
    {
        // Not sleeping:
        if (!(find(sleeping.begin(), sleeping.end(), tid) != sleeping.end()))
        {
            ready.push_back(tid);
        }
        blocked.remove(tid);
        threads[tid]->setState(READY);
    }
    unblock_signals();
    return 0;
}

/*
 * Description: This function blocks the RUNNING thread for usecs micro-seconds in real time (not virtual
 * time on the cpu). It is considered an error if the main thread (tid==0) calls this function. Immediately after
 * the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(unsigned int usec)
{
    block_signals();
    if (running_tid == 0)
    {
        cerr << LIB_ERR << "it's illegal to put the main thread to sleep\n";
        unblock_signals();
        return -1;
    }
    if (usec == 0)
    {
        switch_threads(READY);
        unblock_signals();
        return 0;
    }
    struct timeval etime;
    gettimeofday(&etime, nullptr);
    etime.tv_sec += usec / 1000000;
    etime.tv_usec += usec % 1000000;
    to_wakeup.add(running_tid, etime);
    reset_alarm();
    switch_threads(SLEEP);
    unblock_signals();
    return 0;

}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() { return running_tid; }


/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums() { return total_quantum_num; }


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    block_signals();
    if (notValidTid(tid))
    {
        unblock_signals();
        return -1;
    }
    unblock_signals();
    return threads.at(tid)->getQuantums();
}

