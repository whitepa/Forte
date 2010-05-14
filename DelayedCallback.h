#ifndef __Delayed_callback_h__
#define __Delayed_callback_h__

#include "Thread.h"
#include "Object.h"

namespace Forte
{
    typedef std::multimap<unsigned int, Callback *> CallbackMap;
    typedef std::pair<CallbackMap::iterator,CallbackMap::iterator> CallbackRange;

    EXCEPTION_SUBCLASS(Exception, ForteDelayedCallbackException);

    class DelayedCallback : public Object
    {
        friend class DelayedCallbackThread;
    public:

        static void RegisterCallback(Callback *callback, unsigned int delay);
        static void UnregisterCallback(Callback *callback);

    protected:
        static void StartCallbackThread(void);
        static void StopCallbackThread(void);

        static Mutex sLock;
        static Mutex sThreadLock;
        static Mutex sNotifyLock;
        static ThreadCondition sNotify;
        static Thread *sCallbackThread;

        static std::set<unsigned int> sTimes;
        static CallbackMap sCallbacks;
        static std::map<Callback *, unsigned int> sLookup;
    };

    class DelayedCallbackThread : public Thread {
    public:
        DelayedCallbackThread() {initialized();}
        virtual ~DelayedCallbackThread() {};
    protected:
        void * run(void);
    };
};
#endif
