#include "Forte.h"

std::map<unsigned int, CEventObserver> CEventObserver::sEventObservers;
RWLock CEventObserver::sLock;
unsigned int CEventObserver::sNextID = 0;
CMutex CEventObserver::sNotifyLock;
CCondition CEventObserver::sNotify(CEventObserver::sNotifyLock);
CEventQueue CEventObserver::sQueue(100000, &sNotify); // XXX need unreliable queue which does not block on add()
CThread * CEventObserver::sObserverThread = NULL;

void CEventObserver::broadcastEvent(unsigned int subsysID,
                                    unsigned int eventType,
                                    const char *eventData)
{
    CObservableEvent *e = new CObservableEvent(subsysID, eventType, eventData);
    broadcastEvent(e);
}
void CEventObserver::broadcastEvent(CObservableEvent *event)
{
    CAutoReadUnlock lock(sLock);
    if (!sEventObservers.empty())
        // enqueue the event
        sQueue.add(event);
    else
        delete event;
}
void CEventObserver::runEventCallbacks(CObservableEvent *event)
{
    if (event == NULL) return;
    std::vector<unsigned int> badObservers; // keep a list of bad observers to remove
    {
        CAutoReadUnlock lock(sLock);
        std::map<unsigned int, CEventObserver>::iterator i;
        for (i = sEventObservers.begin(); i != sEventObservers.end(); ++i)
        {
            // call each observer's callback with the event address
            CEventObserver o((*i).second);
            hlog(HLOG_DEBUG, "runEventCallbacks(): event subsysID %d type 0x%x", event->mSubsysID, event->mEventType);
            hlog(HLOG_DEBUG, "runEventCallbacks(): checking observer with subsysID %d and mask 0x%x", o.mSubsysID, o.mTypeMask);
            if (o.mSubsysID == event->mSubsysID && 
                o.mTypeMask & event->mEventType == event->mEventType)
            {
                try
                {
                    event->mObserverID = (*i).first;
                    hlog(HLOG_DEBUG, "running event callback");
                    o.mCallback->execute(event);
                }
                catch (CBadObserverException &e)
                {
                    // this observer needs to be removed
                    badObservers.push_back((*i).first);
                }
                catch (CException &e)
                {
                    hlog(HLOG_ERR, "exception in event observer callback: %s",
                         e.what().c_str());
                }
                catch (...)
                {
                    hlog(HLOG_ERR, "unknown exception in event observer callback");
                }
            }
        }
    }
    std::vector<unsigned int>::iterator i;
    for (i = badObservers.begin(); i != badObservers.end(); ++i)
    {
        hlog(HLOG_WARN, "unregistering event observer ID %u due to errors", *i);
        unregisterObserver(*i);
    }
}

void CEventObserver::startObserverThread(void)
{
    if (sObserverThread != NULL)
        throw CForteEventObserverException("startObserverThread(): sObserverThread not NULL");
    sObserverThread = new CObserverThread();
}
void CEventObserver::stopObserverThread(void)
{
    if (sObserverThread == NULL)
        throw CForteEventObserverException("stopObserverThread(): sObserverThread is NULL");
    delete sObserverThread;
    sObserverThread = NULL;
}
unsigned int CEventObserver::registerObserver(unsigned int subsysID,
                                              unsigned int typeMask,
                                              CCallback *callback)
{
    CAutoWriteUnlock lock(sLock);
    sEventObservers[sNextID] = CEventObserver(subsysID, typeMask, callback);
    if (!sEventObservers.empty() && sObserverThread == NULL)
        startObserverThread();
    return sNextID++;
}

void CEventObserver::unregisterObserver(unsigned int observerID)
{
    CAutoWriteUnlock lock(sLock);
    if (sEventObservers.find(observerID) == sEventObservers.end())
    {
        FString err;
        err.Format("unregisterObserver(): observerID %d does not exist",
                   observerID);
        throw CForteEventObserverException(err);
    }
    if (sEventObservers[observerID].mCallback !=NULL)
        delete sEventObservers[observerID].mCallback;
    sEventObservers.erase(observerID);
}

void *CObserverThread::run(void)
{
    CAutoUnlockMutex lock(CEventObserver::sNotifyLock);
    while(!mThreadShutdown)
    {
        CEvent *event;
        while ((event = CEventObserver::sQueue.get()))
        {
            hlog(HLOG_DEBUG, "observer thread got event");
            CAutoLockMutex unlock(CEventObserver::sNotifyLock);
            CObservableEvent *oe = dynamic_cast<CObservableEvent*>(event);
            if (oe == NULL)
                hlog(HLOG_ERR, "dynamic cast to CObservableEvent failed");
            else
                CEventObserver::runEventCallbacks(oe);
            // now we are done with the event
            delete event;
        }
        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, 0);
        timeout.tv_sec = now.tv_sec + 1; // wake up every second
        timeout.tv_nsec = now.tv_usec * 1000;
        CEventObserver::sNotify.timedwait(timeout);
    }
    return NULL;
}
