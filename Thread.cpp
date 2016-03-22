//
// Created by Danielle on 3/4/2016.
//

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


#include "Thread.h"

Thread::Thread(int threadId, void (*threadFunction)(void))
{
    this->threadId = threadId;
    timeToWake = 0;
    quantumCounter = 0;

    address_t stackPointer, programCounter;
    stackPointer = (address_t)allocatedStack + STACK_SIZE - sizeof(address_t);
    programCounter = (address_t)threadFunction;
    int retval = sigsetjmp(env, 1);
    if(retval == 0)
    {
        env->__jmpbuf[JB_SP] = translate_address(stackPointer);
        env->__jmpbuf[JB_PC] = translate_address(programCounter);
        sigemptyset(&env->__saved_mask);
    }
}

Thread::~Thread()
{

}

int Thread::getThreadId()
{
    return threadId;
}


int Thread::getTimeToWake()
{
    return timeToWake;
}

void Thread::incrementQuantumCounter()
{
    quantumCounter++;
}

sigjmp_buf& Thread::getEnv()
{
    return env;
}

void Thread::setTimeTillWakeUp(int numQuantums)
{
    this->timeToWake = numQuantums;
}

int Thread::getQuantumCounter()
{
    return quantumCounter;
}
