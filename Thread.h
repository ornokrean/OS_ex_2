#ifndef EX2_THREAD_H
#define EX2_THREAD_H
#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define SECOND 1000000
class Thread
{
private:
    int tid;
    char* stack;
    int state;

public:
    Thread(int tid, int stacksize, void (*f)(void));
    int getTID();
};

#endif //EX2_THREAD_H
