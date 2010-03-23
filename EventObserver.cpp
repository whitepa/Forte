#include "Forte.h"

std::map<unsigned int, EventObserver> EventObserver::sEventObservers;
RWLock EventObserver::sLock;
unsigned int EventObserver::sNextID = 0;
Mutex EventObserver::sNotifyLock;
ThreadCondition EventObserver::sNotify(EventObserver::sNotifyLock);
EventQueue EventObserver::sQueue(100000, &sNotify); // XXX need unreliable queue which does not block on add()
Thread * EventObserver::sObserverThread = NULL;

void EventObserver::broadcastEvent(unsigned int subsysID,
                                    unsigned int eventType,
                                    const char *eventData)
{
    ObservableEvent *e = new ObservableEvent(subsysID, eventType, eventData);
    broadcastEvent(e);
}
void EventObserver::broadcastEvent(ObservableEvent *event)
{
    AutoReadUnlock lock(sLock);
    if (!sEventObservers.empty())
        // enqueue the event
        sQueue.add(event);
    else
        delete event;
}
void EventObserver::runEventCallbacks(ObservableEvent *event)
{
    if (event == NULL) return;
    std::vector<unsigned int> badObservers; // keep a list of bad observers to remove
    {
        AutoReadUnlock lock(sLock);
        std::map<unsigned int, EventObserver>::iterator i;
        for (i = sEventObservers.begin(); i != sEventObservers.end(); ++i)
        {
            // call each observer's callback with the event address
            EventObserver o((*i).second);
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
                catch (Exception &e)
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

void EventObserver::startObserverThread(void)
{
    if (sObserverThread != NULL)
        throw ForteEventObserverException("startObserverThread(): sObserverThread not NULL");
    sObserverThread = new ObserverThread();
}
void EventObserver::stopObserverThread(void)
{
    if (sObserverThread == NULL)
        throw ForteEventObserverException("stopObserverThread(): sObserverThread is NULL");
    delete sObserverThread;
    sObserverThread = NULL;
}
unsigned int EventObserver::registerObserver(unsigned int subsysID,
                                              unsigned int typeMask,
                                              Callback *callback)
{
    AutoWriteUnlock lock(sLock);
    sEventObservers[sNextID] = EventObserver(subsysID, typeMask, callback);
    if (!sEventObservers.empty() && sObserverThread == NULL)
        startObserverThread();
    return sNextID++;
}

void EventObserver::unregisterObserver(unsigned int observerID)
{
    AutoWriteUnlock lock(sLock);
    if (sEventObservers.find(observerID) == sEventObservers.end())
    {
        FString err;
        err.Format("unregisterObserver(): observerID %d does not exist",
                   observerID);
        throw ForteEventObserverException(err);
    }
    if (sEventObservers[observerID].mCallback !=NULL)
        delete sEventObservers[observerID].mCallback;
    sEventObservers.erase(observerID);
}

void *ObserverThread::run(void)
{
    AutoUnlockMutex lock(EventObserver::sNotifyLock);
    while(!mThreadShutdown)
    {
        Event *event;
        while ((event = EventObserver::sQueue.get()))
        {
            hlog(HLOG_DEBUG, "observer thread got event");
            AutoLockMutex unlock(EventObserver::sNotifyLock);
            ObservableEvent *oe = dynamic_cast<ObservableEvent*>(event);
            if (oe == NULL)
                hlog(HLOG_ERR, "dynamic cast to ObservableEvent failed");
            else
                EventObserver::runEventCallbacks(oe);
            // now we are done with the event
            delete event;
        }
        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, 0);
        timeout.tv_sec = now.tv_sec + 1; // wake up every second
        timeout.tv_nsec = now.tv_usec * 1000;
        EventObserver::sNotify.timedwait(timeout);
    }
    return NULL;
}
