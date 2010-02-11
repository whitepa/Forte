#ifndef __ReceiverThread_h
#define __ReceiverThread_h

#include "Forte.h"
#include "Event.h"

EXCEPTION_SUBCLASS(CForteException, CForteReceiverThreadException);

class CReceiverThread : public CThread
{
public:
    inline CReceiverThread(CDispatcher &disp, const char *name, const char *bindIP = "") :
        mDisp(disp), mName(name), mBindIP(bindIP)
    {
        if (mName.empty()) throw CException("receiver must be given a valid name");
        // TODO: validate IP address
        initialized();
    }
//    virtual ~CReceiverThread();

protected:
    virtual void * run();
    
    CDispatcher &mDisp;
    FString mName;
    FString mBindIP;
};

#endif
