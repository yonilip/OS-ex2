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
#define MAX_THREAD_NUM 100
#define FAILED -1
#define SUCCESS 0
#define MAIN_THREAD 0

#include "uthreads.h"
#include "Thread.h"
#include <algorithm>
#include <functional>
#include <queue>
#include <iostream>

using namespace std;

deque<Thread*> readyQueue;
priority_queue<int ,vector<int>, greater<int> > tidHeap;

vector<Thread*> sleepingThreads;
vector<Thread*> blockedThreads;
Thread *runningThread;
Thread* threadZero;
sigset_t sigSet;
int totalQuantum;

struct itimerval timer;
struct sigaction sigAction;

//TODO static for all non API funcs
void printAllDAST()
{
	deque<Thread*>::iterator it1;
	vector<Thread*>::iterator it2;
	vector<Thread*>::iterator it3;

	cout << "Running: " << runningThread->getThreadId() << endl;
	cout << "READY: ";
	for(it1 = readyQueue.begin() ; it1 != readyQueue.end() ; ++it1)
	{
		cout <<  (*it1)->getThreadId() << " , ";
	}
	cout << endl << "SLEEPING: ";
	for(it2 = sleepingThreads.begin() ; it2 != sleepingThreads.end() ; ++it2)
	{
		cout << (*it2)->getThreadId()<< " , ";
	}
	cout << endl << "BLOCKED: ";
	for(it3 = blockedThreads.begin() ; it3 != blockedThreads.end() ; ++it3)
	{
		cout <<  (*it3)->getThreadId() << " , ";
	}
	cout << endl;
}

void blockSignals()
{
	if(sigprocmask(SIG_SETMASK, &sigSet, NULL)) //TODO replace this in all places where blocking signals
	{
		cerr << "system error: cannot call sigprogmask " << endl;
		exit(1);
	}
}

void unblockSignalsWithIgnore()
{
	if(sigemptyset(&sigSet))
	{
		cerr << "system error: cannot call sigemptyset " << endl;
		exit(1);
	}; // this will ignore the pending signals
	if(sigaddset(&sigSet, SIGVTALRM))
	{
		cerr << "system error: cannot add signal to set" << endl;
		exit(1);
	}
	if(sigprocmask(SIG_UNBLOCK, &sigSet, NULL))
	{
		cerr << "system error: cannot call sigprogmask " << endl;
		exit(1);
	}
}

void unblockSignal()
{
	if(sigprocmask(SIG_UNBLOCK, &sigSet, NULL))
	{
		cerr << "system error: cannot call sigprogmask " << endl;
		exit(1);
	}
}

void roundRobinAlg()
{

	blockSignals();
	if (runningThread != nullptr)
	{
		int retVal = sigsetjmp(runningThread->getEnv(), 1);
		if (retVal != 0)
		{
			unblockSignal();
			return;
		}
	}


	int totalQuantum = uthread_get_total_quantums();
	Thread* threadToWake;
	if (!sleepingThreads.empty())
	{
		vector<Thread*>::iterator it;
		for (it = sleepingThreads.begin(); it != sleepingThreads.end(); ++it)
		{

			if ((*it)->getTimeToWake() <= totalQuantum)
			{
				threadToWake = (*it);
				sleepingThreads.erase(it);
				readyQueue.push_back(threadToWake);
				if (sleepingThreads.empty())
				{
					break;
				}
			}
		}
	}

	//head of readyQueue will move to running and pop from queue
	if (!readyQueue.empty()){

		// need to check if ptr is null in case of termination
		if(runningThread != nullptr)
		{
			readyQueue.push_back(runningThread);
		}
		runningThread = readyQueue.front();
		readyQueue.pop_front();
	}
	//printAllDAST();

	unblockSignalsWithIgnore();

	if(setitimer(ITIMER_VIRTUAL, &timer, NULL)) // start counting from now
	{
		cerr << "system error: cannot set timer" << endl;
		exit(1);
	}
	//totalQuantum++;
	runningThread->incrementQuantumCounter();
	siglongjmp(runningThread->getEnv(), 1);
}

void timerHandler(int sig)
{
	blockSignals();
	totalQuantum++;
	if (runningThread != nullptr)
	{
		//runningThread->incrementQuantumCounter();
	}
	roundRobinAlg();
}

/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_ucatch signal, total quantum is :secs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
	if(quantum_usecs <= 0)
	{
		cerr << "thread library error: invalid quantum_usecs" << endl;
		return FAILED;
	}

	// init threadZero
	threadZero = new Thread(0, NULL);
	//readyQueue.push_back(threadZero);
	runningThread = threadZero;
	for (int i = 1; i <= 100; ++i)
	{
		tidHeap.push(i);
	}

	totalQuantum = 1;

	if(sigemptyset(&sigSet))
	{
		cerr << "system error: cannot call sigemptyset " << endl;
		exit(1);
	}
	if(sigemptyset(&sigAction.sa_mask))
	{
		cerr << "system error: cannot call sigemptyset " << endl;
		exit(1);
	}
	sigAction.sa_flags = 0;
	sigAction.sa_handler = &timerHandler;
	if(sigaddset(&sigSet, SIGVTALRM))
	{
		cerr << "system error: cannot add signal to set " << endl;
		exit(1);
	}

	// initial timer for the first interval

	timer.it_value.tv_sec = (quantum_usecs/SECOND);
	timer.it_value.tv_usec = (quantum_usecs % SECOND);   //quantum_usecs - (quantum_usecs/SECOND);

	// initial timer for rest of iterations
	timer.it_interval.tv_sec = (quantum_usecs/SECOND);
	timer.it_interval.tv_usec = (quantum_usecs % SECOND);           //quantum_usecs - (quantum_usecs/SECOND);

	if (sigaction(SIGVTALRM, &sigAction, NULL) < 0)
	{
		cerr << "system error: cannot initial sigaction properly" << endl;
		exit(1);
	}

	if(setitimer(ITIMER_VIRTUAL, &timer, NULL))
	{
		cerr << "system error: cannot set timer" << endl;
		exit(1);
	}

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
	blockSignals();
	if ((readyQueue.size() + sleepingThreads.size() + blockedThreads.size() + 1) < MAX_THREAD_NUM)
	{
		int newTid = tidHeap.top();
		tidHeap.pop();
		Thread* newThread = new Thread(newTid, f);

		readyQueue.push_back(newThread);

		unblockSignal();

		return newTid;
	}
	else
	{
		cerr << "thread library error: queque is full, cannot spawn "
				"additional threads" << endl;
		unblockSignal();
		return FAILED;
	}
}

/**
 * @return 1 if tid is positive, else return 0
 */
int validatePositiveTid(int tid)
{
	if(tid > 0)
	{
		return 1;
	}
	return 0;
}


/**
 * @return the iterator that matches tid from readyQueue, else return
 * readyQueue.end()
 */
deque<Thread *>::iterator getThreadIterFromReadyQueue(int tid)
{
	if (!validatePositiveTid(tid))
	{
		return readyQueue.end();
	}
	deque<Thread*>::iterator it;
	if (!readyQueue.empty()){
		for(it = readyQueue.begin() ; it != readyQueue.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				return it;
			}
		}
	}
	return readyQueue.end();
}

/**
 * @return the iterator that matches tid from blockedThreads, else return
 * blockedThreads.end()
 */
vector<Thread *>::iterator getThreadIterFromBlockedVec(int tid)
{
	if (!validatePositiveTid(tid))
	{
		return blockedThreads.end();
	}
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

/**
 * @return the iterator that matches tid from sleepingThreads, else return
 * sleepingThreads.end()
 */
vector<Thread *>::iterator getThreadIterFromSleepingVec(int tid)
{
	if (!validatePositiveTid(tid))
	{
		return sleepingThreads.end();
	}
	vector<Thread*>::iterator it;
	if (!sleepingThreads.empty()){
		for(it = sleepingThreads.begin() ; it != sleepingThreads.end() ; it++)
		{
			if ((*it)->getThreadId() == tid)
			{
				return it;
			}
		}
	}
	return sleepingThreads.end();
}

void freeAll()
{
	blockedThreads.clear();
	sleepingThreads.clear();
	readyQueue.clear();
	try
	{
		delete(runningThread);
	}
	catch (exception)
	{
		cerr << "cannot delete thread ptr " << endl;
	}

	if(sigemptyset(&sigSet))
	{
		cerr << "system error: cannot call sigemptyset" << endl;
		exit(1);
	}
	//stop the timer
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	return;
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
	blockSignals();
	if (tid < 0)
	{
		cerr << "thread library error: invalid tid" << endl;
		unblockSignal();
		return FAILED;
	}
	if (tid == MAIN_THREAD)
	{
		freeAll();
		exit(SUCCESS);
	}

	int deletedThreadID = -1;

	if(runningThread->getThreadId() == tid)
	{
		deletedThreadID = tid;
		delete(runningThread);
		runningThread = nullptr;
		tidHeap.push(deletedThreadID);
		totalQuantum++;
		roundRobinAlg();
	}

	// search for thread in blocked threads vector:
	auto blockedIt = getThreadIterFromBlockedVec(tid);
	if(deletedThreadID == -1 && blockedIt != blockedThreads.end())
	{
		deletedThreadID = (*blockedIt)->getThreadId();
		delete(*blockedIt); // TODO if leak try using temp pointer
		blockedThreads.erase(blockedIt);
	}

	// search for thread in ready queue threads vector:
	auto readyIt = getThreadIterFromReadyQueue(tid);
	if(deletedThreadID == -1 && readyIt != readyQueue.end())
	{
		deletedThreadID = (*readyIt)->getThreadId();
		delete(*readyIt);
		readyQueue.erase(readyIt);
	}

	if(deletedThreadID == -1)
	{
		cerr << "thread library error: thread doesent exist" << endl;
		unblockSignal();
		return FAILED;
	}

	tidHeap.push(deletedThreadID);

	//unblock the signal
	unblockSignal();
	/*totalQuantum++;
	roundRobinAlg();*/
	//TODO us uncommented, this scenario happens if terminate other thread
	return SUCCESS;
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
	blockSignals();
	if (tid < 0)
	{
		cerr << "thread library error: invalid tid" << endl;
		unblockSignal();
		return FAILED;
	}

	if(tid == MAIN_THREAD)
	{
		cerr << "thread library error: cannot block main thread" << endl;
		unblockSignal();
		return FAILED;
	}

	if(runningThread->getThreadId() == tid)
	{
		//runningThread->incrementQuantumCounter();
		Thread* tempThreadp = runningThread;
		blockedThreads.push_back(tempThreadp);
		runningThread = nullptr;
		timerHandler(0);
		return SUCCESS;
	}

	vector<Thread*>::iterator isThreadBlockedOrSleeping;
	isThreadBlockedOrSleeping = getThreadIterFromBlockedVec(tid);
	if(isThreadBlockedOrSleeping != blockedThreads.end())
	{
		unblockSignal();
		return SUCCESS;
	}

	isThreadBlockedOrSleeping = getThreadIterFromSleepingVec(tid);
	if (isThreadBlockedOrSleeping != sleepingThreads.end())
	{
		unblockSignal();
		return SUCCESS;
	}

	deque<Thread *>::iterator threadIterToBlock;
	threadIterToBlock = getThreadIterFromReadyQueue(tid); // method gets thread iter from queue

	if (threadIterToBlock != readyQueue.end())
	{
		Thread* tempThreadp = (*threadIterToBlock);
		readyQueue.erase(threadIterToBlock);
		blockedThreads.push_back(tempThreadp);
		unblockSignal();
		return SUCCESS;
	}
	else
	{
		cerr << "thread library error: thread doesnt exist" << endl;
		unblockSignal();
		return FAILED;
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
	blockSignals();
	if (tid < 0)
	{
		cerr << "thread library error: negative tid" << endl;
		unblockSignal();
		return FAILED;
	}

	vector<Thread *>::iterator threadToResume = getThreadIterFromBlockedVec(
			tid);

	if (threadToResume != blockedThreads.end())
	{
		Thread* tempThread = *threadToResume;
		blockedThreads.erase(threadToResume);
		readyQueue.push_back(tempThread);

		unblockSignal();
		return SUCCESS;
	}

	//Check if even exists
	if (runningThread->getThreadId() == tid || tid == MAIN_THREAD)
	{
		unblockSignal();
		return SUCCESS;
	}

	deque<Thread*>::iterator isThreadFromReady =
			getThreadIterFromReadyQueue(tid);
	if (isThreadFromReady != readyQueue.end())
	{
		unblockSignal();
		return SUCCESS;
	}

	vector<Thread*>::iterator isThreadFromSleep =
			getThreadIterFromSleepingVec(tid);
	if (isThreadFromSleep != sleepingThreads.end())
	{
		unblockSignal();
		return SUCCESS;
	}
	cerr << "thread library error: thread doesnt exist" << endl;
	unblockSignal();
	return FAILED;
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
	blockSignals();
	if (num_quantums <= 0)
	{
		cerr << "thread library error: invalid num_quantums" << endl;
		unblockSignal();
		return FAILED;
	}

	runningThread->setTimeTillWakeUp(num_quantums +
									 uthread_get_total_quantums());
	Thread* tempThread = runningThread;
	runningThread = nullptr;
	sleepingThreads.push_back(tempThread);

	timerHandler(0);
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
	blockSignals();
	if (tid < 0)
	{
		cerr << "thread library error: negative tid" << endl;
		unblockSignal();
		return FAILED;
	}

	vector<Thread*>::iterator isSleepingThread =
			getThreadIterFromSleepingVec(tid);
	if (isSleepingThread != sleepingThreads.end())
	{
		unblockSignal();
		return (*isSleepingThread)->getTimeToWake();
	}

	if (runningThread->getThreadId() == tid)
	{
		unblockSignal();
		return SUCCESS;
	}

	if(getThreadIterFromBlockedVec(tid) != blockedThreads.end())
	{
		unblockSignal();
		return SUCCESS;
	}

	if(getThreadIterFromReadyQueue(tid) != readyQueue.end())
	{
		unblockSignal();
		return SUCCESS;
	}

	unblockSignal();
	return FAILED;
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
	return runningThread->getThreadId();
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
	blockSignals();
	if (tid < 0)
	{
		cerr << "thread library error: negative tid" << endl;
		unblockSignal();
		return FAILED;
	}

	deque<Thread*>::iterator it1;
	vector<Thread*>::iterator it2, it3;

	if (runningThread->getThreadId() == tid)
	{
		unblockSignal();
		return runningThread->getQuantumCounter();
	}
	else if ((it1 = getThreadIterFromReadyQueue(tid)) != readyQueue.end())
	{
		unblockSignal();
		return (*it1)->getQuantumCounter();
	}
	else if ((it2 = getThreadIterFromSleepingVec(tid)) != sleepingThreads.end())
	{
		unblockSignal();
		return (*it2)->getQuantumCounter();
	}
	else if ((it3 = getThreadIterFromBlockedVec(tid)) != blockedThreads.end())
	{
		unblockSignal();
		return (*it3)->getQuantumCounter();
	}
	else
	{
		cerr << "thread library error: thread doesnt exist" << endl;
		unblockSignal();
		return FAILED;
	}
}
