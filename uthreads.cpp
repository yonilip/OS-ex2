
#define SECOND 1000000
#define MAX_THREAD_NUM 100
#define FAILED -1
#define SUCCESS 0
#define MAIN_THREAD 0

#define END_OF_PROCESS 2

#include "uthreads.h"
#include "Thread.h"
#include <algorithm>
#include <queue>
#include <iostream>

using namespace std;

deque<Thread*> readyQueue;
priority_queue<int ,vector<int>, greater<int> > tidHeap;

vector<Thread*> sleepingThreads;
vector<Thread*> blockedThreads;
Thread *runningThread;
Thread *suicideThread;
Thread* threadZero;
sigset_t sigSet;
int totalQuantum;

struct itimerval timer;
struct sigaction sigAction;


/**
 * block the signals that are set in the sigset
 */
void blockSignals()
{
	if(sigprocmask(SIG_SETMASK, &sigSet, NULL))
		// places where blocking signals
	{
		cerr << "system error: cannot call sigprogmask " << endl;
		exit(1);
	}
}

/**
 * unblocks the signals that were in sigset and removes pending signals that
 * were caught during run
 */
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

/**
 * unblocks the signal. pending signals may be invoked after this
 */
void unblockSignal()
{
	if(sigprocmask(SIG_UNBLOCK, &sigSet, NULL))
	{
		cerr << "system error: cannot call sigprogmask " << endl;
		exit(1);
	}
}

/**
 * Implementation of the round robin algorithm
 * Context switch happens here
 */
void roundRobinAlg()
{
	blockSignals();
	if (runningThread != nullptr)
	{
		int retVal = sigsetjmp(runningThread->getEnv(), 1);
		if (retVal == END_OF_PROCESS)
		{
			delete(runningThread);
			exit(SUCCESS);
		}
		if (retVal != 0)
		{
			try
			{
				delete(suicideThread);
				suicideThread = nullptr;
			}
			catch (exception e)
			{
				cerr << "thread library error: cannot delete thread" << endl;
				unblockSignal();
				return;
			}
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

	unblockSignalsWithIgnore();
	if(setitimer(ITIMER_VIRTUAL, &timer, NULL)) // start counting from now
	{
		cerr << "system error: cannot set timer" << endl;
		exit(1);
	}

	runningThread->incrementQuantumCounter();
	siglongjmp(runningThread->getEnv(), 1);
}

/**
 * The signal handler for SIGVTALRM
 */
void timerHandler(int sig)
{
	if(sig == SIGVTALRM || sig == 0)
	{
		blockSignals();
		totalQuantum++;
		roundRobinAlg();
	}
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
	try
	{
		threadZero = new Thread(0, NULL);
	}
	catch (exception e)
	{
		cerr << "thread library error: cannot spawn new thread" << endl;
		unblockSignal();
		return FAILED;
	}
	runningThread = threadZero;
	//create the tid's and push to heap
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
	timer.it_value.tv_usec = (quantum_usecs % SECOND);

	// initial timer for rest of iterations
	timer.it_interval.tv_sec = (quantum_usecs/SECOND);
	timer.it_interval.tv_usec = (quantum_usecs % SECOND);

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
	if ((readyQueue.size() + sleepingThreads.size() +
		 blockedThreads.size() + 1) < MAX_THREAD_NUM)
	{
		int newTid = tidHeap.top();
		tidHeap.pop();
		Thread* newThread;
		try
		{
			newThread = new Thread(newTid, f);
		}
		catch (exception e)
		{
			cerr << "thread library error: cannot aloocate memory for thread"
			<< endl;
			unblockSignal();
			return FAILED;
		}
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
	Thread* tempThread = nullptr;
	if (runningThread->getThreadId() != MAIN_THREAD)
	{
		//main thread will be in readyQueue
		auto mainThreadIt = getThreadIterFromReadyQueue(MAIN_THREAD);
		tempThread = (*mainThreadIt);
		//erase does not free aloocated memory since vector holds pointers
		readyQueue.erase(mainThreadIt);
	}
	blockedThreads.clear();
	sleepingThreads.clear();
	readyQueue.clear();

	//cleanup before termination
	if(sigemptyset(&sigSet))
	{
		cerr << "system error: cannot call sigemptyset" << endl;
		exit(1);
	}
	//stop the timer
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	//if running isn't main, switch to main thread to safetly delete runningh thread
	if (tempThread != nullptr)
	{
		//jump to main thread
		siglongjmp(tempThread->getEnv(), END_OF_PROCESS);
	}


	//if running == main then exit
	exit(SUCCESS);
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
	}

	int deletedThreadID = -1;

	if(runningThread->getThreadId() == tid)
	{
		deletedThreadID = tid;
//		try
//		{
//			delete(runningThread);
//		}
//		catch (exception e)
//		{
//			cerr << "thread library error: cannot delete thread" << endl;
//			unblockSignal();
//			return FAILED;
//		}
		suicideThread = runningThread;
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
		try
		{
			delete(*blockedIt);
		}
		catch (exception e )
		{
			cerr << "thread library error: cannot delete thread" << endl;
			unblockSignal();
			return FAILED;
		}
		blockedThreads.erase(blockedIt);
	}

	// search for thread in ready queue threads vector:
	auto readyIt = getThreadIterFromReadyQueue(tid);
	if(deletedThreadID == -1 && readyIt != readyQueue.end())
	{
		deletedThreadID = (*readyIt)->getThreadId();
		try
		{
			delete(*readyIt);
		}
		catch (exception e)
		{
			cerr << "thread library error: cannot delete thread" << endl;
			unblockSignal();
			return FAILED;
		}
		readyQueue.erase(readyIt);
	}

	if(deletedThreadID == -1)
	{
		cerr << "thread library error: thread doesent exist" << endl;
		unblockSignal();
		return FAILED;
	}

	tidHeap.push(deletedThreadID);
	unblockSignal();
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
		Thread* tempThreadp = runningThread;
		int retVal = sigsetjmp(runningThread->getEnv(), 1);
		if (retVal == 0)
		{
			blockedThreads.push_back(tempThreadp);
			runningThread = nullptr;
			timerHandler(0);
		}
		unblockSignal();
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
	threadIterToBlock = getThreadIterFromReadyQueue(tid);
	// method gets thread iter from queue

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

	if(runningThread->getThreadId() != MAIN_THREAD) {
		runningThread->setTimeTillWakeUp(num_quantums +
										 uthread_get_total_quantums());
		Thread *tempThread = runningThread;
		int retVal = sigsetjmp(runningThread->getEnv(), 1);
		if (retVal == 0)
		{
			runningThread = nullptr;
			sleepingThreads.push_back(tempThread);
			timerHandler(0);
		}
		unblockSignal();
		return SUCCESS;
	}
	cerr << "thread library error: cannot send main thread to sleep" << endl;
	unblockSignal();
	return FAILED;
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
 * Return value: On success, return the number of quantums of the thread with
 * ID tid. On failure, return -1.
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
	else if ((it2 = getThreadIterFromSleepingVec(tid)) !=
			 sleepingThreads.end())
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
