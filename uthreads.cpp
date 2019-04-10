#include <bits/types/timer_t.h>
#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <list>
#include <vector>
#include <iostream>
#include "Thread.h"
#include <setjmp.h>
#include <unistd.h>

#define SYS_ERR "system error: "
#define LIB_ERR "thread library error: "


//============================ Globals ============================//
using namespace std;

int total_quantum_num = 1; //total number of quantums started
int running_tid = 0; // The id of the currently running thread
int num_of_threads = 0; // The total number of threads


struct itimerval timer; // The timer duh
struct sigaction sa;
sigjmp_buf env[MAX_THREAD_NUM];

list<int> ready; // Queue for all ready threads
list<int> blocked; // Queue for blocked threads
vector<Thread *> threads;


//============================ Helper Functions ============================//

/*
 * Description: Returns the first empty id for a thread. If no thread is empty, returns -1.
*/

int notValidTid(int tid)
{
    if (tid < 0 || threads.at(tid) == nullptr)
    {
        return -1;
    }
    return 0;
}

int getFirstID()
{
    for (int i = 1; i < MAX_THREAD_NUM; ++i)
    {
        if (threads.at(i) == nullptr)
        {
            return i;
        }
    }
    return -1;
}

void switch_threads(int to_state)
{
    if (threads[running_tid] != nullptr)
    {
        int ret_val = sigsetjmp(*threads[running_tid]->getEnv(), 1); // Save the current to_state of the process
        if (ret_val != 0)
        { // If returning from another process, exit and continue the run normally
            return;
        }

        threads[running_tid]->setState(to_state);
        if (to_state==READY)
        {
            // Push the currently running thread to the end of the ready queue
            ready.push_back(running_tid);
        }
        else if (to_state==BLOCKED){
            // Push the currently running thread to the end of the blocked queue
            blocked.push_back(running_tid);
            threads[running_tid]->setState(BLOCKED);
        }

        // Update the total number of quantums and the quantums run by the specific thread.
//        threads[running_tid]->addQuanta();
        total_quantum_num++;

        // Set the next running thread to be the first in the ready queue
        running_tid = ready.front();
        ready.pop_front();
        threads[running_tid]->setState(RUNNING);

//        for (auto v : blocked)
//            std::cout << v << "\n";


        // Start running the next process:
        siglongjmp(*threads[running_tid]->getEnv(), 1);
    }

}

void timer_handler(int sig)
{
    switch_threads(READY);
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
    if (quantum_usecs <= 0) { return -1; }
    threads.resize(MAX_THREAD_NUM);
    threads.at(0) = new Thread(0, STACK_SIZE, nullptr);
    threads[0]->setState(RUNNING);

    //total_quantum_num = 1;

    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        printf("sigaction error.");
    }
    // first time interval, seconds part
    timer.it_value.tv_sec = quantum_usecs / 1000000;
    // first time interval, microseconds part
    timer.it_value.tv_usec = quantum_usecs % 1000000;
    // following time intervals, seconds part
    timer.it_interval.tv_sec = quantum_usecs / 1000000;
    // following time intervals, microseconds part
    timer.it_interval.tv_usec = quantum_usecs % 1000000;

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        printf("setitimer error.");
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

    if (num_of_threads == MAX_THREAD_NUM) { return -1; }
    int newtid = getFirstID();
    threads[newtid] = new Thread(newtid, STACK_SIZE, f);
    ready.push_back(newtid);


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

    if (notValidTid(tid))
    {
        return -1;
    }

    // do it anyway
    Thread *toDelete = threads.at(tid);
    delete (toDelete);
    threads[tid] = nullptr;

    if (tid == 0)
    {
//        do some shit
//        free all allocs
        exit(0);
    }
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
    if (tid == 0)
    {
        cout << LIB_ERR << "Trying to block main thread (tid==0).\n";
        return -1;
    }
    if (notValidTid(tid))
    {
        cout << LIB_ERR << "No thread with id " << tid << " exists.\n";
        return -1;
    }
    //Case: Thread is running:
    if (tid == running_tid)
    {
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
    if (notValidTid(tid)){
        cout << LIB_ERR << "No thread with id " << tid << " exists.\n";
        return -1;
    }
    //Case: Thread is blocked:
    if (threads[tid]->getState() == BLOCKED)
    {
        blocked.remove(tid);
        ready.push_back(tid);
    }
    return 0;
}

/*
 * Description: This function blocks the RUNNING thread for usecs micro-seconds in real time (not virtual
 * time on the cpu). It is considered an error if the main thread (tid==0) calls this function. Immediately after
 * the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(unsigned int usec){
    if (running_tid==0){
        cout<<LIB_ERR<<"Main thread can't sleep. Its El Pacino.";
        return  -1;
    }
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


    if (notValidTid(tid))
    {
        cout << LIB_ERR << "No thread with id " << tid << " exists.\n";
        return -1;
    }
    return threads.at(tid)->getQuantums();
}

