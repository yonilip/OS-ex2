//
// Created by Danielle on 3/4/2016.
//

#ifndef UNTITLED1_THREAD_H
#define UNTITLED1_THREAD_H

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>

#define STACK_SIZE 4096


class Thread
{
private:

    int threadId;
    sigjmp_buf env;

    int quantumCounter; // inc every time this thread is in running state
    int timeToWake;
    char allocatedStack[STACK_SIZE];


public:
    Thread(int threadId, void (*threadFunction)(void));

    ~Thread();

    int getThreadId();

    int getQuantumCounter();
    int getTimeToWake();


    void incrementQuantumCounter();

    sigjmp_buf& getEnv();

    void setTimeTillWakeUp(int numQuantums);

};

#endif //UNTITLED1_THREAD_H
