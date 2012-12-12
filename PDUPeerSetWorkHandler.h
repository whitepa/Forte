#ifndef __Forte_PDUPeerSetWorkHandler_h
#define __Forte_PDUPeerSetWorkHandler_h

#include "RequestHandler.h"
#include "PDUPeerSet.h"
#include "Event.h"

namespace Forte
{
    class PDUPeerSetWorkHandler : public Forte::RequestHandler
    {
    public:
        PDUPeerSetWorkHandler();
        virtual ~PDUPeerSetWorkHandler();

    public:
        // RequestHandler Methods
        virtual void Handler(Forte::Event *e);
        virtual void Periodic() {}
        virtual void Busy() {}
        virtual void Init() {}
        virtual void Cleanup() {}
        // End RequestHandler Methods
    };
    typedef boost::shared_ptr<PDUPeerSetWorkHandler> PDUPeerSetWorkHandlerPtr;
}
#endif
