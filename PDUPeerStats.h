#ifndef __Forte_PDUPeerStats_h_
#define __Forte_PDUPeerStats_h_

#include "Object.h"

namespace Forte
{
    struct PDUPeerStats
    {
    public:
        PDUPeerStats()
            : totalSent(0),
              totalReceived(0),
              totalQueued(0),
              bytesSent(0),
              sendErrors(0)
            {}

        int64_t totalSent;
        int64_t totalReceived;
        int64_t totalQueued;
        int64_t bytesSent;
        int64_t sendErrors;
        //TODO:
        //int64_t averageQueueSize;
    };

    struct PDUPeerSetStats
    {
        PDUPeerSetStats()
            : connectedCount(0)
            {}

        int64_t connectedCount;
        std::map<uint64_t, PDUPeerStats> pduPeerStats;
    };
};

#endif
