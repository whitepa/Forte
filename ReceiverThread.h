#ifndef __ReceiverThread_h
#define __ReceiverThread_h

#include "Dispatcher.h"
#include "Exception.h"
#include "Event.h"
#include "FString.h"

namespace Forte
{
    EXCEPTION_CLASS(EReceiverThread);

    EXCEPTION_SUBCLASS(EReceiverThread,
                       EReceiverThreadNameInvalid);

    EXCEPTION_SUBCLASS2(EReceiverThread,
                        EReceiverDispatcherInvalid,
                        "Dispatcher is invalid");

    EXCEPTION_SUBCLASS2(EReceiverThread,
                        ERecieverThreadPollAdd,
                        "Failed to add descriptor to poll socket");

    EXCEPTION_SUBCLASS2(EReceiverThread,
                        EReceiverThreadPollCreate,
                        "Failed to create poll socket");

    EXCEPTION_SUBCLASS(EReceiverThread, EReceiverThreadPollFailed);

    class ReceiverThread : public Thread
    {
    public:
        inline ReceiverThread(
            boost::shared_ptr<Dispatcher> disp,
            const char *name,
            int port,
            int backlog,
            const char *bindIP = "")
            :  mDisp(disp),
               mName(name),
               mPort(port),
               mBacklog(backlog),
               mBindIP(bindIP)
            {
                if (!disp)
                    throw EReceiverDispatcherInvalid();

                if (mName.empty())
                    throw Exception("receiver must be given a valid name");

                // TODO: validate IP address
                initialized();
            }

        virtual ~ReceiverThread() {
            deleting();
        }

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
