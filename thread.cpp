#include "thread.h"


#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
        "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

Thread::Thread(unsigned int tid, int stacksize, void (*f)(void))
{
    this->tid = tid;
    this->stack = new char[stacksize];
    this->state = READY;
    this->quantums = 0;
    if (tid != 0)
    {
        address_t sp, pc;

        sp = (address_t) stack + stacksize - sizeof(address_t);
        pc = (address_t) f;
        sigsetjmp(env, 1);
        (env->__jmpbuf)[JB_SP] = translate_address(sp);
        (env->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&env->__saved_mask);
    }


}

Thread::~Thread()
{
    delete[] this->stack;
}

unsigned int Thread::getTID()
{ return this->tid; }

int Thread::getState()
{ return this->state; }

void Thread::setState(int state)
{
    if (state == READY || state == BLOCKED || state == RUNNING)
    {
        this->state = state;
        if (state == RUNNING)
        {
            this->quantums++;
        }
    }

}

sigjmp_buf *Thread::getEnv()
{
    return &this->env;
}

int Thread::getQuantums()
{ return this->quantums; }



