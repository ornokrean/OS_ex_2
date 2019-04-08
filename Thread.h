#ifndef EX2_THREAD_H
#define EX2_THREAD_H
#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define SECOND 1000000
class Thread
{
private:
    unsigned int tid;
    char* stack;
    int state;
    int quantums;

public:
    Thread(unsigned int tid, int stacksize, void (*f)(void));
    unsigned int getTID();
    int getState();

};

#endif //EX2_THREAD_H
