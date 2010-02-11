#include "Forte.h"

// this system uses a 1 second granularity, and waked up every second to look for callbacks to make
// (this was done out of ease of implementation)
// therefore: minimum delay is 1 second (might be anywhere from 0 - 1 second)

CMutex CDelayedCallback::sLock;
CMutex CDelayedCallback::sThreadLock;
CMutex CDelayedCallback::sNotifyLock;
CThreadCondition CDelayedCallback::sNotify(CDelayedCallback::sNotifyLock);
CThread * CDelayedCallback::sCallbackThread;

std::set<unsigned int> CDelayedCallback::sTimes;
CallbackMap CDelayedCallback::sCallbacks;
std::map<CCallback *, unsigned int> CDelayedCallback::sLookup;



void CDelayedCallback::registerCallback(CCallback *callback, unsigned int delay)
{
    CAutoUnlockMutex lock(sLock);
    // insert new callback into array of callbacks, sorted by execution time
    time_t now = time(0);
    time_t runtime = now + delay;
    hlog(HLOG_DEBUG, "registerCallback(): now %u; registering a callback for time %u",
         now, runtime);
    sTimes.insert(runtime);
    sCallbacks.insert(pair<unsigned int, CCallback*>(runtime,callback));
    sLookup.insert(pair<CCallback*,unsigned int>(callback,runtime));
    if (sCallbackThread == NULL)
        startCallbackThread();
}

void CDelayedCallback::unregisterCallback(CCallback *callback)
{
    CAutoUnlockMutex lock(sLock);
    // remove from array
    std::map<CCallback*,unsigned int>::iterator i;
    i = sLookup.find(callback);
    if (i == sLookup.end())
    {
        hlog(HLOG_ERR,"unregisterCallback(): callback not in lookup map");
        return;
    }
    unsigned int callbackTime = (*i).second;

    CallbackRange range = sCallbacks.equal_range(*(sTimes.begin()));
    unsigned int count = sCallbacks.count(*(sTimes.begin()));
    CallbackMap::iterator ci;
    // XXX some potential for optimization here, 
    // we are traversing a list looking for a match in O(n) time
    for (ci = range.first; ci != range.second; ++ci)
    {
        CCallback *c = (*ci).second;
        if (c == callback)
        {
            sLookup.erase((*ci).second);
            sCallbacks.erase(ci);
            break;
        }
    }
    if (count == 1)
        sTimes.erase(callbackTime);
    if (sTimes.empty())
    {
        hlog(HLOG_DEBUG, "stopping callback thread");
        stopCallbackThread();
    }
}

void CDelayedCallback::startCallbackThread(void)
{
    CAutoUnlockMutex lock(sThreadLock);
    if (sCallbackThread != NULL)
        throw CForteDelayedCallbackException("sCallbackThread not NULL");
    sCallbackThread = new CDelayedCallbackThread();
}
void CDelayedCallback::stopCallbackThread(void)
{
    CAutoUnlockMutex lock(sThreadLock);
    if (sCallbackThread == NULL)
        throw CForteDelayedCallbackException("sCallbackThread is NULL");
    delete sCallbackThread;
    sCallbackThread = NULL;
}

void * CDelayedCallbackThread::run(void)
{
    mThreadName.Format("dly-callback");
    hlog(HLOG_DEBUG, "delayed callback thread starting");
    CAutoUnlockMutex lock(CDelayedCallback::sNotifyLock);
    while (!mThreadShutdown)
    {
        std::list<CCallback *> executeList;
        {
            time_t now = time(0);
            CAutoUnlockMutex dataLock(CDelayedCallback::sLock);
//            hlog(HLOG_DEBUG, "delayed callback thread:  next execute time is %u", (CDelayedCallback::sTimes.size() != 0)?*(CDelayedCallback::sTimes.begin()):0);
            std::set<unsigned int>::iterator ti;
            ti = CDelayedCallback::sTimes.begin();
            while (ti != CDelayedCallback::sTimes.end() && *ti <= (unsigned)now)
            {
                CallbackRange range = CDelayedCallback::sCallbacks.equal_range(*ti);
                CallbackMap::iterator i;
                for (i = range.first; i != range.second; ++i)
                {
                    hlog(HLOG_DEBUG, "time is %u, executing callback for time %u", now, (*i).first);
                    CCallback *c = (*i).second;
                    executeList.push_back(c);
                    CDelayedCallback::sLookup.erase((*i).second);
                    CDelayedCallback::sCallbacks.erase(i);
                }
                hlog(HLOG_DEBUG, "out of range loop");
                CDelayedCallback::sTimes.erase(ti);
                ti = CDelayedCallback::sTimes.begin();
            }
        }
        std::list<CCallback *>::iterator i;
        // execute all the callbacks here (after data has been unlocked)
        // (this is done to avoid deadlock if a callback calls unregisterCallback() on itself)
        for (i = executeList.begin(); i!=executeList.end(); ++i)
        {
            hlog(HLOG_DEBUG, "executing a callback");
            (*i)->execute(0);
        }
//        hlog(HLOG_DEBUG, "done with execution, waiting 1 second");
        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, 0);
        timeout.tv_sec = now.tv_sec + 1; // wake up every second
        timeout.tv_nsec = now.tv_usec * 1000;
        CDelayedCallback::sNotify.timedwait(timeout);
    }
    hlog(HLOG_DEBUG, "delayed callback thread stopping");
    return NULL;
}
