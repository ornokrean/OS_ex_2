#ifndef EX2_THREAD_H
#define EX2_THREAD_H


class Thread
{
private:
    int tid;
    char* stack;
    int state;

public:
    Thread(int tid, int stacksize);
    int getTID();
};

#endif //EX2_THREAD_H
