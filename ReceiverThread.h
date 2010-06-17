#ifndef __ReceiverThread_h
#define __ReceiverThread_h

#include "Forte.h"
#include "Event.h"

namespace Forte
{
    EXCEPTION_CLASS(EReceiverThread);
    EXCEPTION_SUBCLASS(EReceiverThread, EReceiverThreadNameInvalid);
    EXCEPTION_SUBCLASS2(EReceiverThread, EReceiverDispatcherInvalid, "Dispatcher is invalid");

    class ReceiverThread : public Thread
    {
    public:
        inline ReceiverThread(shared_ptr<Dispatcher> disp, const char *name, 
                              int port, int backlog, const char *bindIP = "") :
            mDisp(disp), mName(name), 
            mPort(port), mBacklog(backlog), mBindIP(bindIP)
            {
                if (!disp) throw EReceiverDispatcherInvalid();
                if (mName.empty()) throw Exception("receiver must be given a valid name");
                // TODO: validate IP address
                initialized();
            }
//    virtual ~ReceiverThread();

    protected:
        virtual void * run();
    
        boost::shared_ptr<Dispatcher> mDisp;
        FString mName;
        int mPort;
        int mBacklog;
        FString mBindIP;
    };
};
#endif
