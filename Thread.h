//
// Created by Danielle on 3/4/2016.
//

#ifndef UNTITLED1_THREAD_H //TODO fix this
#define UNTITLED1_THREAD_H

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>



#define STACK_SIZE 4096


enum State {Ready, Running, Blocked, Sleeping, Terminated};

class Thread
{
private:

    unsigned int threadId;
    State state; // TODO check if needed
    //TODO check if needed alloction for thread stack

    sigjmp_buf env;

    //void sigMaskSet; //TODO change type
    unsigned int quantumCounter; // inc every time this thread is in running state
    int timeToWake; //TODO change type
    char* allocatedStack;


public:
    Thread(unsigned int threadId, void (*threadFunction)(void));

    ~Thread();

    const unsigned int getThreadId();
    const State getState();
    void setState(const State state);

    int getqQuantumCounter();
    int getTimeToWake();

    void incrementQuantumCounter();

    void resetSleepingTime();

    sigjmp_buf& getEnv();

    void setTimeTillWakeUp(int numQuantums);

};

#endif //UNTITLED1_THREAD_H
