#ifndef EX2_THREAD_H
#define EX2_THREAD_H

#include <bits/types/sigset_t.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define SECOND 1000000
#define READY 0
#define BLOCKED 1
#define RUNNING 2


class Thread
{
private:
    unsigned int tid;
    char* stack;
    int state;
    int quantums;
    sigjmp_buf env;

public:
    Thread(unsigned int tid, int stacksize, void (*f)(void));
    ~Thread();
    unsigned int getTID();
    int getState();
    void setState(int state);
    sigjmp_buf* getEnv();
    int getQuantums();
//    void addQuanta();

};

#endif //EX2_THREAD_H
