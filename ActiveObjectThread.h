#ifndef __Forte__Active_Object_Thread_h__
#define __Forte__Active_Object_Thread_h__

#include "AsyncInvocation.h"
#include "Exception.h"
#include "EventQueue.h"
#include "Thread.h"

namespace Forte
{
    EXCEPTION_CLASS(EActiveObjectThread);
    EXCEPTION_SUBCLASS2(EActiveObjectThread, EActiveObjectThreadReceivedInvalidEvent,
                        "Active Object Thread received incorrect event type");
    EXCEPTION_SUBCLASS2(EActiveObjectThread, EActiveObjectNoCurrentInvocation,
                        "Active Object has no current invocation");
    EXCEPTION_SUBCLASS2(EActiveObjectThread, EActiveObjectThreadShuttingDown,
                        "Active Object thread shutting down");
    
    class ActiveObjectThread : public Forte::Thread
    {
    public:
        ActiveObjectThread();
        virtual ~ActiveObjectThread();
        
        virtual void * run(void);

        virtual void Enqueue(const boost::shared_ptr<AsyncInvocation> &ai);

        virtual void CancelRunning(void);

        virtual bool IsCancelled(void);

        virtual void SetName(const FString &name);

        virtual void DropQueue(void);

        virtual void WaitUntilQueueEmpty(void);

    private:
        Forte::EventQueue mQueue;
        Forte::Mutex mLock;
        shared_ptr<Forte::AsyncInvocation> mCurrentAsyncInvocation;
    };
    
};

#endif
