#include "uthreads.h"
#include <iostream>

int gotit1 = 0;
int gotit2 = 0;
int gotit3 = 1;
int gotit4 = 0;
int gotit5 = 0;
int gotit6 = 0;
int gotit7 = 0;

void f(void) {
    while (1) {
        if (gotit1) {
            std::cout << "in f: ";
            std::cout <<"quantums:"<< uthread_get_quantums(uthread_get_tid()) << std::endl;
            gotit1 = 0;
            gotit2 = 1;
            gotit3 = 1;

        }
    }
}

void g(void) {
    while (1) {
        if (gotit2) {
            std::cout << "in g :";
            std::cout << "quantums:"<< uthread_get_quantums(uthread_get_tid())<< std::endl;

            gotit2 = 0;
            gotit1 = 1;
            gotit3 = 1;

            if((5 <= uthread_get_total_quantums()) && uthread_get_total_quantums() <=7)
            {
                uthread_block(1);
            }

            if((8<= uthread_get_total_quantums()))
            {
                if(uthread_resume(1))
                {
                    std::cout << "Cant do that ! " << std::endl;
                }
            }

        }
    }
}

//
int main() {
    uthread_init(1000000);
    uthread_spawn(f);
    uthread_spawn(g);

    for (;;) {
        if (gotit3) {
//            print_ready();
            std::cout << "in main total: ";
            std::cout << uthread_get_total_quantums()  << std::endl;

            std::cout << "in main : ";
            std::cout << uthread_get_quantums(uthread_get_tid())  << std::endl;



            gotit3 = 0;
            gotit1 = 1;
            gotit2 = 1;

        }

    }
}