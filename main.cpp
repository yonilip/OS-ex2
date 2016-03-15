//
// Created by danielle.kut on 3/9/16.
//


#include <sys/unistd.h>
#include "Thread.h"
#include "uthreads.h"
#define SECOND 1000000

void f(void)
{
    int i = 0;
    while(1){
        ++i;
        printf("in f (%d)\n",i);
    }
}


int main()
{
    uthread_init(30000000);
    uthread_spawn(f);
}