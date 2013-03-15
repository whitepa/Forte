#ifndef _Forte_RequestHandler_h
#define _Forte_RequestHandler_h

#include "Object.h"

namespace Forte
{
    // base class for a user-defined request handler
    class Event;

    class RequestHandler : public Object {
    public:
        inline RequestHandler(unsigned int timeout = 0) : mTimeout(timeout) {};
        inline virtual ~RequestHandler() {};
        virtual void Handler(Event *e) = 0;
        virtual void Busy(void) = 0;
        virtual void Periodic(void) = 0;
        // called by each thread when it starts
        virtual void Init(void) = 0;
        virtual void Cleanup(void) = 0;
        unsigned int mTimeout;
        // XXX add acceptable event type mask
    };
};
#endif
