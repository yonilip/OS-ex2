//
// Created by Danielle on 3/4/2016.
//

#ifndef UNTITLED1_THREAD_H //TODO fix this
#define UNTITLED1_THREAD_H

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>

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

    int getTimeToWake();

    void incrementQuantumCounter();

    void resetSleepingTime();

    sigjmp_buf& getEnv();

    void setTimeTillWakeUp(int numQuantums);

};

#endif //UNTITLED1_THREAD_H
