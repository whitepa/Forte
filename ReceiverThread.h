#ifndef __ReceiverThread_h
#define __ReceiverThread_h

#include "Forte.h"
#include "Event.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(ForteException, ForteReceiverThreadException);

    class ReceiverThread : public Thread
    {
    public:
        inline ReceiverThread(Dispatcher &disp, const char *name, const char *bindIP = "") :
            mDisp(disp), mName(name), mBindIP(bindIP)
            {
                if (mName.empty()) throw Exception("receiver must be given a valid name");
                // TODO: validate IP address
                initialized();
            }
//    virtual ~ReceiverThread();

    protected:
        virtual void * run();
    
        Dispatcher &mDisp;
        FString mName;
        FString mBindIP;
    };
};
#endif
