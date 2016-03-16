//
// Created by danielle.kut on 3/16/16.
//

/**
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
 */