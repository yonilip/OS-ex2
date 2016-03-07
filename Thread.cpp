//
// Created by Danielle on 3/4/2016.
//

#include "Thread.h"

enum State {Ready, Running, Blocked, Sleeping, Terminated};

class Thread
{
private:
    unsigned int threadId;
    State state;
    address_t stackPointer;
    //TODO check if needed alloction for thread stack
    void sigMaskSet; //TODO change type
    unsigned int quantumCounter; // inc every time this thread is in running state
    void startedSleepTime; //TODO change type
    void (*threadFunction)(void);
    //TODO should the thread recieve a pointer to the function that it runs?

public:
    Thread(unsigned int threadId, void (*threadFunction)(void))
    {
        this->threadId = threadId;
        this->state = Ready;
        startedSleepTime = 0;
        quantumCounter = 0;
        this->threadFunction = threadFunction; //TODO is this ref, deref or like this?
        //TODO init sigMaskSet, stackPointer
    }

    ~Thread()
    {
       //TODO should we free something?
    }

    const unsigned int getThreadId()
    {
        return threadId;
    }

    const State getState()
    {
        return state;
    }

    void setState(const State state)
    {
        this->state = state; // when running inc quantumCounter by 1. check if this will inc here or in move to running func
        /*
         * maybe switch statement here for different given states that does stuff such as invoking inc method
         */
    }

    void getStartedSleepTime()
    {
        return startedSleepTime;
    }

    void incrementQuantumCounter()
    {
        quantumCounter++;
    }

    void resetSleepingTime()
    {
        startedSleepTime = 0;
    }


    //TODO setSleepTime, state change's func,

    // terminated



};