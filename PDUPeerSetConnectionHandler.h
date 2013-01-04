// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPeerSetConnectionHandler_h
#define __Forte_PDUPeerSetConnectionHandler_h

#include "RequestHandler.h"
#include "PDUPeerSet.h"
#include "Event.h"

namespace Forte
{
    class PDUPeerSetConnectionHandler : public Forte::RequestHandler
    {
    public:
        PDUPeerSetConnectionHandler(const Forte::PDUPeerSetPtr& peerSet);
        virtual ~PDUPeerSetConnectionHandler();

    public:
        // RequestHandler Methods
        virtual void Handler(Forte::Event *e);
        virtual void Periodic() {}
        virtual void Busy() {}
        virtual void Init() {}
        virtual void Cleanup() {}
        // End RequestHandler Methods

    protected:
        Forte::PDUPeerSetPtr mPDUPeerSet;
    };
}
#endif
