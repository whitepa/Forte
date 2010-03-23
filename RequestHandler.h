#ifndef _RequestHandler_h
#define _RequestHandler_h

namespace Forte
{
// base class for a user-defined request handler
    class RequestHandler : public Object {
    public:
        inline RequestHandler(unsigned int timeout = 0) : mTimeout(timeout) {};
        inline virtual ~RequestHandler() {};
        virtual void handler(Event *e) = 0;
        virtual void busy(void) = 0;
        virtual void periodic(void) = 0;
        virtual void init(void) = 0;
        virtual void cleanup(void) = 0;
        unsigned int mTimeout;
        // XXX add acceptable event type mask
    };
};
#endif
