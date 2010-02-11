#ifndef __Delayed_callback_h__
#define __Delayed_callback_h__

#include "Thread.h"

typedef std::multimap<unsigned int, CCallback *> CallbackMap;
typedef std::pair<CallbackMap::iterator,CallbackMap::iterator> CallbackRange;

EXCEPTION_SUBCLASS(CForteException, CForteDelayedCallbackException);

class CDelayedCallback
{
    friend class CDelayedCallbackThread;
public:

    static void registerCallback(CCallback *callback, unsigned int delay);
    static void unregisterCallback(CCallback *callback);

protected:
    static void startCallbackThread(void);
    static void stopCallbackThread(void);

    static CMutex sLock;
    static CMutex sThreadLock;
    static CMutex sNotifyLock;
    static CThreadCondition sNotify;
    static CThread *sCallbackThread;

    static std::set<unsigned int> sTimes;
    static CallbackMap sCallbacks;
    static std::map<CCallback *, unsigned int> sLookup;

};

class CDelayedCallbackThread : public CThread {
public:
    CDelayedCallbackThread() {initialized();}
    virtual ~CDelayedCallbackThread() {};
protected:
    void * run(void);
};

#endif
