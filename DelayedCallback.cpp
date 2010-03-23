#include "Forte.h"

// this system uses a 1 second granularity, and waked up every second to look for callbacks to make
// (this was done out of ease of implementation)
// therefore: minimum delay is 1 second (might be anywhere from 0 - 1 second)

Mutex DelayedCallback::sLock;
Mutex DelayedCallback::sThreadLock;
Mutex DelayedCallback::sNotifyLock;
ThreadCondition DelayedCallback::sNotify(DelayedCallback::sNotifyLock);
Thread * DelayedCallback::sCallbackThread;

std::set<unsigned int> DelayedCallback::sTimes;
CallbackMap DelayedCallback::sCallbacks;
std::map<Callback *, unsigned int> DelayedCallback::sLookup;



void DelayedCallback::registerCallback(Callback *callback, unsigned int delay)
{
    AutoUnlockMutex lock(sLock);
    // insert new callback into array of callbacks, sorted by execution time
    time_t now = time(0);
    time_t runtime = now + delay;
    hlog(HLOG_DEBUG, "registerCallback(): now %u; registering a callback for time %u",
         now, runtime);
    sTimes.insert(runtime);
    sCallbacks.insert(pair<unsigned int, Callback*>(runtime,callback));
    sLookup.insert(pair<Callback*,unsigned int>(callback,runtime));
    if (sCallbackThread == NULL)
        startCallbackThread();
}

void DelayedCallback::unregisterCallback(Callback *callback)
{
    AutoUnlockMutex lock(sLock);
    // remove from array
    std::map<Callback*,unsigned int>::iterator i;
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
        Callback *c = (*ci).second;
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

void DelayedCallback::startCallbackThread(void)
{
    AutoUnlockMutex lock(sThreadLock);
    if (sCallbackThread != NULL)
        throw ForteDelayedCallbackException("sCallbackThread not NULL");
    sCallbackThread = new DelayedCallbackThread();
}
void DelayedCallback::stopCallbackThread(void)
{
    AutoUnlockMutex lock(sThreadLock);
    if (sCallbackThread == NULL)
        throw ForteDelayedCallbackException("sCallbackThread is NULL");
    delete sCallbackThread;
    sCallbackThread = NULL;
}

void * DelayedCallbackThread::run(void)
{
    mThreadName.Format("dly-callback");
    hlog(HLOG_DEBUG, "delayed callback thread starting");
    AutoUnlockMutex lock(DelayedCallback::sNotifyLock);
    while (!mThreadShutdown)
    {
        std::list<Callback *> executeList;
        {
            time_t now = time(0);
            AutoUnlockMutex dataLock(DelayedCallback::sLock);
//            hlog(HLOG_DEBUG, "delayed callback thread:  next execute time is %u", (DelayedCallback::sTimes.size() != 0)?*(DelayedCallback::sTimes.begin()):0);
            std::set<unsigned int>::iterator ti;
            ti = DelayedCallback::sTimes.begin();
            while (ti != DelayedCallback::sTimes.end() && *ti <= (unsigned)now)
            {
                CallbackRange range = DelayedCallback::sCallbacks.equal_range(*ti);
                CallbackMap::iterator i;
                for (i = range.first; i != range.second; ++i)
                {
                    hlog(HLOG_DEBUG, "time is %u, executing callback for time %u", now, (*i).first);
                    Callback *c = (*i).second;
                    executeList.push_back(c);
                    DelayedCallback::sLookup.erase((*i).second);
                    DelayedCallback::sCallbacks.erase(i);
                }
                hlog(HLOG_DEBUG, "out of range loop");
                DelayedCallback::sTimes.erase(ti);
                ti = DelayedCallback::sTimes.begin();
            }
        }
        std::list<Callback *>::iterator i;
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
        DelayedCallback::sNotify.timedwait(timeout);
    }
    hlog(HLOG_DEBUG, "delayed callback thread stopping");
    return NULL;
}
