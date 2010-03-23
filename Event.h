#ifndef _Event_h
#define _Event_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "FString.h"
#include "Object.h"

namespace Forte
{
    class Event : public Object {
    public:
        Event() {};
        Event(unsigned int userTypeInfo) : mUserTypeInfo(userTypeInfo) {};
        virtual ~Event() {}
        virtual Event * copy() = 0;
        int mServerID;
        struct timeval mStartTime;
        unsigned int mUserTypeInfo;
    };

    class RequestEvent : public Event {
    public:
        RequestEvent() {};
        virtual ~RequestEvent() {}
        int mFd;
        struct in_addr mClient;
        struct timeval mTime;

        virtual Event *copy() {
            RequestEvent *e = new RequestEvent;
            e->mServerID = mServerID;
            e->mStartTime = mStartTime;
            e->mUserTypeInfo = mUserTypeInfo;
            e->mFd = mFd;
            e->mClient = mClient;
            e->mTime = mTime;
            return e;
        }
    };

    class ObservableEvent : public Event {
    public:
        ObservableEvent() {};
        ObservableEvent(unsigned int subsysID,
                         unsigned int eventType,
                         const char *eventData) :
            mSubsysID(subsysID),
            mEventType(eventType),
            mEventData(eventData) {};
        virtual ~ObservableEvent() {}
    
        unsigned int mSubsysID;
        unsigned int mEventType;
        unsigned int mObserverID; // set differently as handed to each observer
        FString mEventData;
    
        virtual Event *copy() {
            ObservableEvent *e = new ObservableEvent;
            e->mServerID = mServerID;
            e->mStartTime = mStartTime;
            e->mUserTypeInfo = mUserTypeInfo;
            e->mSubsysID = mSubsysID;
            e->mEventType = mEventType;
            e->mEventData = mEventData;
            return e;
        }
    };
};
#endif
