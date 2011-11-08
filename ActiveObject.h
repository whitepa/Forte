#ifndef __Forte_ActiveObject_h__
#define __Forte_ActiveObject_h__

#include "ActiveObjectThread.h"
#include <boost/make_shared.hpp>

namespace Forte
{
    EXCEPTION_CLASS(EActiveObject);
    EXCEPTION_SUBCLASS2(EActiveObject, EActiveObjectShuttingDown,
                        "Active Object is shutting down");
    /**
     * ActiveObject
     *
     * Method calls on an ActiveObject return Futures, which can be
     * queried directly for status and/or results of the given method.
     */
    class ActiveObject : virtual public Forte::Object
    {
    public:
        ActiveObject() : 
            mActiveObjectThreadPtr(new Forte::ActiveObjectThread()) {};
        virtual ~ActiveObject() {Shutdown(true,false);}

        template<typename ResultType>
        shared_ptr<Forte::Future<ResultType> > InvokeAsync(
            boost::function<ResultType(void)> callback) {
            if (!mActiveObjectThreadPtr)
                throw_exception(EActiveObjectShuttingDown());
            shared_ptr<Future<ResultType> > future(make_shared<Future<ResultType> >());
            mActiveObjectThreadPtr->Enqueue(
                make_shared<ConcreteInvocation<ResultType> >(future, callback));
            return future;
        }
        bool IsCancelled(void) {
            if (!mActiveObjectThreadPtr)
                throw_exception(EActiveObjectShuttingDown());
            return mActiveObjectThreadPtr->IsCancelled();
        }

        void SetThreadName(const FString &name){
            if (!mActiveObjectThreadPtr)
                throw_exception(EActiveObjectShuttingDown());
            mActiveObjectThreadPtr->SetName(name);
        }

        void Shutdown(bool waitForQueueDrain = true, 
                      bool cancelRunning = false) {
            if (!mActiveObjectThreadPtr)
                return;
            mActiveObjectThreadPtr->Shutdown();
            if (!waitForQueueDrain)
                mActiveObjectThreadPtr->DropQueue();
            if (cancelRunning)
                mActiveObjectThreadPtr->CancelRunning();
            mActiveObjectThreadPtr->WaitForShutdown();
            mActiveObjectThreadPtr.reset();
        }
    private:
        boost::scoped_ptr<Forte::ActiveObjectThread> mActiveObjectThreadPtr;
    };

};

#endif
