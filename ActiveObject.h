#ifndef __Forte_ActiveObject_h__
#define __Forte_ActiveObject_h__

#include "ActiveObjectThread.h"
#include <boost/make_shared.hpp>

namespace Forte
{

    /**
     * ActiveObject
     *
     * Method calls on an ActiveObject return Futures, which can be
     * queried directly for status and/or results of the given method.
     */
    class ActiveObject : virtual public Forte::Object
    {
    public:
        virtual ~ActiveObject() {};

    protected:
        template<typename ResultType>
        shared_ptr<Forte::Future<ResultType> > invokeAsync(
            boost::function<ResultType(void)> callback) {
            shared_ptr<Future<ResultType> > future(make_shared<Future<ResultType> >());
            mActiveObjectThread.Enqueue(
                make_shared<ConcreteInvocation<ResultType> >(future, callback));
            return future;
        }
        bool isCancelled(void) {
            return mActiveObjectThread.IsCancelled();
        }

        void setThreadName(const FString &name){
            mActiveObjectThread.SetName(name);
        }
    private:
        Forte::ActiveObjectThread mActiveObjectThread;
    };

};

#endif
