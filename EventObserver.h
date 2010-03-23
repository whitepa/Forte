#ifndef __EventObserver_h
#define __EventObserver_h

// EventObserver manages event observers
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
    EXCEPTION_SUBCLASS(ForteException, ForteEventObserverException);
    EXCEPTION_SUBCLASS(ForteEventObserverException, CBadObserverException);

    class EventObserver : public Object {
        friend class ObserverThread;
    public:
        EventObserver() {};
        EventObserver(unsigned int subsysID,
                       unsigned int typeMask, Callback *callback) :
            mSubsysID(subsysID), mTypeMask(typeMask), mCallback(callback) {};
        virtual ~EventObserver() {};
        static void broadcastEvent(ObservableEvent *event);
        static void broadcastEvent(unsigned int subsysID, 
                                   unsigned int eventType, 
                                   const char *eventData);

        static void runEventCallbacks(ObservableEvent *event);

        static unsigned int registerObserver(unsigned int subsysID,
                                             unsigned int typeMask, 
                                             Callback *callback);
        static void unregisterObserver(unsigned int observerID);


    protected:
        static void startObserverThread(void);
        static void stopObserverThread(void);

        static std::map<unsigned int, EventObserver> sEventObservers;
        static unsigned int sNextID;
        static RWLock sLock;
        static EventQueue sQueue;
        static Mutex sNotifyLock;
        static ThreadCondition sNotify;
        static Thread *sObserverThread;

        unsigned int mSubsysID; // unsigned int identifying the desired subsystem
        unsigned int mTypeMask; // bitmask flagging desired events
        Callback *mCallback; // method to call with event ptr
    };

    class ObserverThread : public Thread {
    public:
        ObserverThread() { initialized(); }
        virtual ~ObserverThread() {};
    protected:
        void *run(void);
    };
};
#endif
