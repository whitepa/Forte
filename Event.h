#ifndef _Event_h
#define _Event_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "FString.h"
#include "Object.h"
#include <boost/function.hpp>

namespace Forte
{
    class Event : public Object {
    public:
        Event() {};
        Event(const FString &name) : mServerID(-1), mName(name) {}
        virtual ~Event() {}

        const FString &GetName(void) const { return mName; }

        int mServerID;
        struct timeval mStartTime;
        FString mName;
    };

    class RequestEvent : public Event {
    public:
        RequestEvent() : mFD(-1) {}
        virtual ~RequestEvent() {}
        int mFD;
        struct in_addr mClient;
        struct timeval mTime;
    };

    class PDUEvent : public Event
    {
    public:
        PDUEvent() {}
        virtual ~PDUEvent() {}

        virtual void DoWork() = 0;
    };

};
#endif
