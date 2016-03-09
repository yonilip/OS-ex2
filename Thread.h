//
// Created by Danielle on 3/4/2016.
//

#ifndef UNTITLED1_THREAD_H //TODO fix this
#define UNTITLED1_THREAD_H


//
// Created by Danielle on 3/4/2016.
//


#define STACK_SIZE 4096


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
    Thread(unsigned int threadId, void (*threadFunction)(void));

    ~Thread();

    const unsigned int getThreadId();
    const State getState();
    void setState(const State state);

//    void getStartedSleepTime()
//    {
//        return startedSleepTime;
//    }

    void incrementQuantumCounter();

    void resetSleepingTime();

};

#endif //UNTITLED1_THREAD_H
