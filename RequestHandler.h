#ifndef _RequestHandler_h
#define _RequestHandler_h

namespace Forte
{
// base class for a user-defined request handler
    class RequestHandler : public Object {
    public:
        inline RequestHandler(unsigned int timeout = 0) : mTimeout(timeout) {};
        inline virtual ~RequestHandler() {};
        virtual void Handler(Event *e) = 0;
        virtual void Busy(void) = 0;
        virtual void Periodic(void) = 0;
        virtual void Init(void) = 0;
        virtual void Cleanup(void) = 0;
        unsigned int mTimeout;
        // XXX add acceptable event type mask
    };
};
#endif
