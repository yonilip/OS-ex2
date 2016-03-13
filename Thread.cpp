//
// Created by Danielle on 3/4/2016.
//


#include "Thread.h"

Thread::Thread(unsigned int threadId, void (*threadFunction)(void))
{
    this->threadId = threadId;
    this->state = Ready;
    startedSleepTime = 0;
    quantumCounter = 0;
    allocatedStack = new char[STACK_SIZE];

    address_t stackPointer, programCounter;
    stackPointer = (address_t)allocatedStack + STACK_SIZE - sizeof(address_t);
    programCounter = (address_t)threadFunction;
    sigsetjmp(env, 1); //TODO what is this 1 savemask thing?
    env->__jmpbuf[JB_SP] = translate_address(stackPointer);
    env->__jmpbuf[JB_PC] = translate_address(programCounter);
    sigemptyset(env->__saved_mask);

    /**
     * timer shit follows
     */


}

Thread::~Thread()
{
    //TODO should we free something?
    free(this->stackPointer);
}

const unsigned int Thread::getThreadId()
{
    return threadId;
}

const State Thread::getState()
{
    return state;
}

void Thread::setState(const State state)
{
    this->state = state; // when running inc quantumCounter by 1. check if this will inc here or in move to running func
    /*
     * maybe switch statement here for different given states that does stuff such as invoking inc method
     */
}

//    void getStartedSleepTime()
//    {
//        return startedSleepTime;
//    }

void Thread::incrementQuantumCounter()
{
    quantumCounter++;
}

void Thread::resetSleepingTime()
{
    startedSleepTime = 0;
}

sigjmp_buf& getEnv()
{
    return env;
}

//TODO setSleepTime, state change's func,

// terminated
void Thread::setTimeTillWakeUp(int numQuantums)
{
    this->timeUntilWakeUp = numQuantums;
}
