#ifndef _Event_h
#define _Event_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "FString.h"
namespace Forte
{
    class CEvent : public Object {
    public:
        CEvent() {};
        CEvent(unsigned int userTypeInfo) : mUserTypeInfo(userTypeInfo) {};
        virtual ~CEvent() {}
        virtual CEvent * copy() = 0;
        int mServerID;
        struct timeval mStartTime;
        unsigned int mUserTypeInfo;
    };

    class CRequestEvent : public CEvent {
    public:
        CRequestEvent() {};
        virtual ~CRequestEvent() {}
        int mFd;
        struct in_addr mClient;
        struct timeval mTime;

        virtual CEvent *copy() {
            CRequestEvent *e = new CRequestEvent;
            e->mServerID = mServerID;
            e->mStartTime = mStartTime;
            e->mUserTypeInfo = mUserTypeInfo;
            e->mFd = mFd;
            e->mClient = mClient;
            e->mTime = mTime;
            return e;
        }
    };

    class CObservableEvent : public CEvent {
    public:
        CObservableEvent() {};
        CObservableEvent(unsigned int subsysID,
                         unsigned int eventType,
                         const char *eventData) :
            mSubsysID(subsysID),
            mEventType(eventType),
            mEventData(eventData) {};
        virtual ~CObservableEvent() {}
    
        unsigned int mSubsysID;
        unsigned int mEventType;
        unsigned int mObserverID; // set differently as handed to each observer
        FString mEventData;
    
        virtual CEvent *copy() {
            CObservableEvent *e = new CObservableEvent;
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
