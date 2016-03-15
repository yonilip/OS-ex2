/*
 * Ok here is the rough design of the shit we are doing:
 *
 * Class Thread:
 * 		const unsigned int threadId
 * 		state (Part of the enum that holds states: READY, SLEEPING, BLOCKED, RUNNING)
 * 		stackPointer (of type address_t)
 * 		sigMaskSet
 * 		unsigned int quantumCounter; (increments by 1 each time thread runs)
 * 		sleepTimer (????)
 *
 *
 * 		Optional:
 * 		priority
 *
 *
 * Manager/Scheduler (main thread?):
 *
 * 		curRunningThread (ptr)
 * 		quantum_usecs (comes from the init func and defines how much time each thread runs)
 * 		unsigned int threadsCounter (counts the amount of spawned threads that havent been terminated)
 * 		int sumQuantumCounter ( counter of all the threads that have been in running state, updated at preemption/start of thread run)
 *
 * 		DAST:
 *
 * 		readyQueue (updates at preemption
 * 		tidMinHeap (for extracting the minimal available tid, should be initialized in the init func. insert tid node when terminating a thread)
 * 		SleepingVector (at preemption check if sleeping thread should move to readyQueue)
 * 		BlockedVector
 *
 * 		Optional:
 * 		existingThreads
 *
 *
 *
 *
 * Stuff we arent sure about:
 * 		Signals and their managers (sigaction and shit)
 * 		time measuring
 * 		exceptions and handling them (does each thread handle its own exception or do they move to the manager?)
 *
 *
 *
 * 		Regarding preemption in the RR-Alg:
 * 			blocking suggestion: when deciding to block a thread, we save the
 * 				state of the current thread using sigSetJump and go to the other
 * 				thread and evaluate what situation it is supposed to handle (i.e run? block myself? wake up?)
 * 				then after blocking self sigLongJump to the thread that blocked this thread
 * 				and continue normally. if Blocking self, go to preemption while updating needed
 * 				DAST's.
 */


#define SECOND 1000000
#define MAX_THREAD_NUM 100 //TODO check the number
#define FAILED -1
#define SUCCESS 0
#define MAIN_THREAD 0

#include "uthreads.h"
#include "Thread.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <setjmp.h>
#include <vector>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <iostream>

using namespace std;

deque<Thread*> readyQueue;
priority_queue<unsigned int ,vector<unsigned int>, greater<unsigned int> > tidHeap;

vector<Thread*> sleepingThreads;
vector<Thread*> blockedThreads;
Thread *runningThread;
Thread* threadZero;
sigset_t sigSet;
int totalQuantum;

struct itimerval timer;
struct sigaction sigAction;

void roundRobinAlg()
{
	while(true)
	{
		int retVal = sigsetjmp(runningThread->getEnv(), 1);
		if (retVal != 0)
		{
			//TODO stop blocking signals!
			return;
		}

		int totalQuantum = uthread_get_total_quantums();
		Thread* threadToWake;
		if (!sleepingThreads.empty())
		{


			for (vector<Thread*>::iterator it = sleepingThreads.begin();
				 it != sleepingThreads.end(); ++it)
			{
				if ((*it)->getTimeToWake() <= totalQuantum)
				{
					threadToWake = *it;
					sleepingThreads.erase(it);
					readyQueue.push_back(threadToWake);
					//TODO maybe change state
				}
			}
		}



		//head of readyQueue will move to running and pop from queue
		if (!readyQueue.empty()){

			readyQueue.push_back(runningThread);

			runningThread = readyQueue.front();
			readyQueue.pop_front();
		}
		//TODO change state?


		//before trying to pull from ready, check if not empty

		// sigwait()
		// siglongjump to make sure time will start in a new quantum (happens in the signal catcher)
		//goto next thread
		sigemptyset(&sigSet); // this will ignore the pending signals
		sigprocmask(SIG_UNBLOCK, &sigSet, NULL);


		if(setitimer(ITIMER_VIRTUAL, &timer, NULL)) // start counting from now
		{
			cout << "cannot set timer " << runningThread->getThreadId() << endl;
			//return FAILED;
		}

		siglongjmp(runningThread->getEnv(), 1);
	}
}


void timerHandler(int sig)
{
	/*int retVal = sigsetjmp(runningThread->getEnv(), 1);
	if(retVal == 0)
	{
		roundRobinAlg();
	}*/
	cout << "catch signal, total quantum is : " << uthread_get_total_quantums() << runningThread->getThreadId() << endl;
	sigprocmask(SIG_SETMASK, &sigSet, NULL);
	totalQuantum++;
	roundRobinAlg();

	/*sigprocmask(SIG_UNBLOCK, &sigSet, NULL);
	//todo wait for next signal
	siglongjmp(runningThread->getEnv(), 1);*/

}

/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
	if(quantum_usecs <= 0)
	{
		cout << "invalid quantum_usecs" << endl;
		return FAILED;
	}

	// init threadZero
	threadZero = new Thread(0, NULL);
	cout << "init Thread zero" << endl;
	readyQueue.push_back(threadZero);
	for (int i = 1; i <= 100; ++i)
	{
		tidHeap.push((const unsigned int &) i);
	}

	sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGVTALRM);

	totalQuantum = 0;

	sigAction.sa_handler = &timerHandler;
	sigAction.sa_flags = 0;

	// initial timer for the first interval
	timer.it_value.tv_sec = (quantum_usecs / SECOND);
	timer.it_value.tv_usec = quantum_usecs - (quantum_usecs/SECOND);

	// initial timer for rest of iterations
	timer.it_interval.tv_sec = (quantum_usecs);
	timer.it_interval.tv_usec = quantum_usecs - (quantum_usecs/SECOND);


	if (sigaction(SIGVTALRM, &sigAction, NULL) < 0)
	{
		cout << "cannot initial gigaction properly" << endl;
		//TODO print err?
		return FAILED;
	}
/*	if (sigaction(SIGSEGV, &sigAction, NULL) < 0)
	{
		cout << "cannot initial gigaction properly" << endl;
		//TODO print err?
		return FAILED;
	}*/

	if(setitimer(ITIMER_VIRTUAL, &timer, NULL))
	{
		cout << "cannot set timer" << endl;
		return FAILED;
	}


	runningThread = readyQueue.front();
	readyQueue.pop_back();
	/*	sigsetjmp(runningThread->getEnv(), 1);
		siglongjmp(runningThread->getEnv(), 1);*/
	roundRobinAlg();
	return SUCCESS;
}


/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
	if (readyQueue.size() < MAX_THREAD_NUM)
	{
		unsigned int newTid = tidHeap.top();
		tidHeap.pop();
		cout << "init new thread with tid :" << newTid << endl;
		Thread* newThread = new Thread(newTid, f);

		sigemptyset(&newThread->getEnv()->__saved_mask);
		readyQueue.push_back(newThread);

		return newTid;
	}
	else
	{
		cout << "queque is full, cannot spawn additional threads" << endl;
		return FAILED;
	}


}
/*


Thread* getThreadFromDAST(int tid)
{
	Thread* threadToFind;

	if (runningThread->getThreadId() == tid) return runningThread; //TODO what happens if the running thread terminates itself? what happens next?

	if (!readyQueue.empty()){
		for(deque<Thread*>::iterator it = readyQueue.begin() ; it != readyQueue.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				threadToFind = *it;
				readyQueue.erase(it);
				return threadToFind;
			}
		}
	}

	if (!sleepingThreads.empty()){
		for(auto it = sleepingThreads.begin() ; it != sleepingThreads.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				threadToFind = *it;
				sleepingThreads.erase(it);
				return threadToFind;
			}
		}
	}

	if (!blockedThreads.empty()){
		for(auto it = blockedThreads.begin() ; it != blockedThreads.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				threadToFind = *it;
				blockedThreads.erase(it);
				return threadToFind;
			}
		}
	}

	// TODO think about impl of this function. do we really need to search all DAST? so we need to pop the thread any time?
	// TODO kefel code
	return nullptr;
}
*/

/**
 * TODO DOC
 */
bool validatePositiveTid(int tid)
{
	if (tid > 0) return true;
}

/**
 * TODO DOC
 */
deque<Thread *>::iterator getThreadIterFromReadyQueue(int tid)
{
	if (!validatePositiveTid(tid))
	{
		return readyQueue.end();
	}
	//Thread* threadToFind = nullptr;
	deque<Thread*>::iterator it;
	if (!readyQueue.empty()){
		for(it = readyQueue.begin() ; it != readyQueue.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				//threadToFind = *it;
				//readyQueue.erase(it);
				//return threadToFind;
				return it;
			}
		}
	}
	return it;
}

/**
 * TODO DOC
 */
Thread *removeThreadFromBlockedQueue(int tid)
{
	if (!validatePositiveTid(tid))
	{
		return NULL;
	}
	Thread* threadToFind = NULL;
	if (!blockedThreads.empty()){
		for(vector<Thread*>::iterator it = blockedThreads.begin() ; it !=
											   blockedThreads.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				threadToFind = *it;
				blockedThreads.erase(it);
				return threadToFind;
			}
		}
	}
	return threadToFind;
}

/**
 * TODO DOC
 */
Thread *removeThreadFromSleepingQueue(int tid)
{
	if (!validatePositiveTid(tid))
	{
		return nullptr;
	}
	Thread* threadToFind = nullptr;
	if (!sleepingThreads.empty()){
		for(vector<Thread*>::iterator it = sleepingThreads.begin() ; it !=
												sleepingThreads.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				threadToFind = *it;
				sleepingThreads.erase(it);
				return threadToFind;
			}
		}
	}
	return threadToFind;
}

void freeAll()
{

/*	Thread* curThread;
	for (int i = 0; i <= MAX_THREAD_NUM ; ++i) {
		curThread = getThreadFromDAST(i);
		if (curThread != nullptr)
		{
			cout << "delete thread with pid : " << curThread->getThreadId() << endl;
			delete(curThread);
		}
	}
	sigemptyset(&sigSet);
	//TODO kill timer
	//TODO release all dasts*/
}

/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered as an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
	/*
	 * need to block signals! until we finish the temination.
	 *
	 */
/*	if (!validatePositiveTid(tid))
	{
		//TODO ERROR!!!
		return FAILED;
	}
	if (tid == MAIN_THREAD)
	{
		freeAll();
		exit(SUCCESS);
	}
	//TODO what happens when terminating running thread? need to block signals!
	Thread* threadToDel = getThreadFromDAST(tid); // removes thread from the container it came from
	if (threadToDel == nullptr) return FAILED; //TODO should we throw err?
	delete(threadToDel);
	cout << "delete thread with pid : " << threadToDel->getThreadId() << endl;
	tidHeap.push((const unsigned int &) tid);
	//unblock the signal*/
}



/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED or SLEEPING states has no
 * effect and is not considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
	/*
	 * Only ready and running threads could be blocked!!
	 */
	//TODO should we block signals?
	if (!validatePositiveTid(tid))
	{
		return FAILED; // TODO err
	}
	deque<Thread *>::iterator threadIterToBlock;
	//threadIterToBlock = getThreadFromDAST(tid);
	if(runningThread->getThreadId() == tid)
	{
		blockedThreads.push_back(runningThread);
		// TODO make scheduling decision
		cout << "block thread with pid : " << (*threadIterToBlock)->getThreadId() << endl;
		return SUCCESS;
	}

	threadIterToBlock = getThreadIterFromReadyQueue(tid); // method gets thread iter from queue

	if (threadIterToBlock == readyQueue.end())
	{
		//cout << "block thread with pid : " << (*threadIterToBlock)->getThreadId() << endl;
		return SUCCESS;
	}
	Thread* tempThreadp = (*threadIterToBlock);
	readyQueue.erase(threadIterToBlock);
	blockedThreads.push_back(tempThreadp);
	return SUCCESS;
}


vector<Thread *>::iterator getThreadIterFromBlockedQueue(int tid)
{
	vector<Thread *>::iterator it;
	for (it = blockedThreads.begin() ; it != blockedThreads.end(); it++)
	{
		if ((*it)->getThreadId() == tid)
		{
			return it;
		}
	}
	return blockedThreads.end();
}

bool isTidInitialized(int tid)
{
	if (tid < 0)
	{
		return false;
	}

	if (runningThread->getThreadId() == tid)
	{
		return true;
	}


}

/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in the RUNNING, READY or SLEEPING
 * state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
	if (isTidInitialized(tid)) //TODO check if tid exists at all!
	{
		//TODO perror
		return FAILED;
	}

	vector<Thread *>::iterator threadToResume =
			getThreadIterFromBlockedQueue(tid);

	if (threadToResume == blockedThreads.end())
	{
		return SUCCESS;
	}

	Thread* tempThread = *threadToResume;
	blockedThreads.erase(threadToResume);
	readyQueue.push_back(tempThread);
	//TODO change state?
	return SUCCESS;
}


/*
 * Description: This function puts the RUNNING thread to sleep for a period
 * of num_quantums (not including the current quantum) after which it is moved
 * to the READY state. num_quantums must be a positive number. It is an error
 * to try to put the main thread (tid==0) to sleep. Immediately after a thread
 * transitions to the SLEEPING state a scheduling decision should be made.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums)
{
	//TODO should we block signals?
	//TODO manager method + thread inner state change
	if (num_quantums <= 0)
	{
		//TODO throw error? print error?
		return FAILED;
	}
	runningThread->setTimeTillWakeUp(num_quantums +
									 uthread_get_total_quantums());
	sleepingThreads.push_back(runningThread);
	//TODO wait for RR where new runningThread will be assigned

	return SUCCESS;

}


/*
 * Description: This function returns the number of quantums until the thread
 * with id tid wakes up including the current quantum. If no thread with ID
 * tid exists it is considered as an error. If the thread is not sleeping,
 * the function should return 0.
 * Return value: Number of quantums (including current quantum) until wakeup.
*/
int uthread_get_time_until_wakeup(int tid)
{
	//TODO return the time remains for thread to sleep

}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
	//TODO return private field

}


/*
 * Description: This function returns the total number of quantums that were
 * started since the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
	return totalQuantum;
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered as an error.
 * Return value: On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
	return runningThread->getqQuantumCounter();
}



