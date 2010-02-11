#ifndef __EventObserver_h
#define __EventObserver_h

// CEventObserver manages event observers
// A single thread is created when observers exist
// All other threads may call broadcastEvent() at any time, passing it
// an event for any interested observers.  Incoming events are placed into
// a queue, which is read by the observer manager thread.  This thread then
// makes all necessary callbacks with the event.

// define some observable event types for forte
#include <map>
#include "Callback.h"
#include "Event.h"
#include "AutoMutex.h"
#include "RWLock.h"


// Subsystems 0 - 255 are reserved by forte library
#define EO_SUBSYS_SERVER        0
namespace Forte
{
    EXCEPTION_SUBCLASS(CForteException, CForteEventObserverException);
    EXCEPTION_SUBCLASS(CForteEventObserverException, CBadObserverException);

    class CEventObserver {
        friend class CObserverThread;
    public:
        CEventObserver() {};
        CEventObserver(unsigned int subsysID,
                       unsigned int typeMask, CCallback *callback) :
            mSubsysID(subsysID), mTypeMask(typeMask), mCallback(callback) {};
        virtual ~CEventObserver() {};
        static void broadcastEvent(CObservableEvent *event);
        static void broadcastEvent(unsigned int subsysID, 
                                   unsigned int eventType, 
                                   const char *eventData);

        static void runEventCallbacks(CObservableEvent *event);

        static unsigned int registerObserver(unsigned int subsysID,
                                             unsigned int typeMask, 
                                             CCallback *callback);
        static void unregisterObserver(unsigned int observerID);


    protected:
        static void startObserverThread(void);
        static void stopObserverThread(void);

        static std::map<unsigned int, CEventObserver> sEventObservers;
        static unsigned int sNextID;
        static RWLock sLock;
        static CEventQueue sQueue;
        static CMutex sNotifyLock;
        static CThreadCondition sNotify;
        static CThread *sObserverThread;

        unsigned int mSubsysID; // unsigned int identifying the desired subsystem
        unsigned int mTypeMask; // bitmask flagging desired events
        CCallback *mCallback; // method to call with event ptr
    };

    class CObserverThread : public CThread {
    public:
        CObserverThread() { initialized(); }
        virtual ~CObserverThread() {};
    protected:
        void *run(void);
    };
};
#endif
