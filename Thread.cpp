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
    this->threadFunction = threadFunction; //TODO is this ref, deref or like this?
    //TODO init sigMaskSet, stackPointer
    try {
        this->stackPointer = malloc(STACK_SIZE); //TODO is this the impl needed?

    } catch (exception e) {
        //TODO catch the bad_alloc thing
    }


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


//TODO setSleepTime, state change's func,

// terminated
