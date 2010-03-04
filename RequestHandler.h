#ifndef _RequestHandler_h
#define _RequestHandler_h

namespace Forte
{
// base class for a user-defined request handler
    class CRequestHandler : public Object {
    public:
        inline CRequestHandler(unsigned int timeout = 0) : mTimeout(timeout) {};
        inline virtual ~CRequestHandler() {};
        virtual void handler(CEvent *e) = 0;
        virtual void busy(void) = 0;
        virtual void periodic(void) = 0;
        virtual void init(void) = 0;
        virtual void cleanup(void) = 0;
        unsigned int mTimeout;
        // XXX add acceptable event type mask
    };
};
#endif
