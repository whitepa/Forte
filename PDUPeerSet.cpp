#include "PDUPeerSet.h"
#include "Foreach.h"
#include "LogManager.h"

using namespace boost;
using namespace Forte;

shared_ptr<PDUPeer> Forte::PDUPeerSet::PeerGet(int fd)
{
    FDMap::iterator i;
    if ((i = mFDMap.find(fd))==mFDMap.end())
        throw EPDUPeerInvalid();
    return (*i).second;
}

void Forte::PDUPeerSet::PeerDisconnected(int fd)
{
    // delete the peer object
    shared_ptr<PDUPeer> peer(PeerGet(fd));
    // erase from FD map
    mFDMap.erase(fd);
    // // erase from IP map
    // IPMap::iterator i;
    // for (i = mIPMap.begin(); i != mIPMap.end();)
    // {
    //     shared_ptr<PDUPeer> p((*i).second);
    //     if (p->getFD() == fd)
    //         mIPMap.erase(i++);
    //     else
    //         ++i;
    // }
}
void Forte::PDUPeerSet::PeerConnected(int fd)
{
    shared_ptr<PDUPeer> peer(new PDUPeer(fd));
    mFDMap[fd] = peer;
    
}

void Forte::PDUPeerSet::SendAll(PDU &pdu)
{
    foreach (const FDPair &p, mFDMap)
    {
        PDUPeer &peer(*(p.second));
        hlog(HLOG_DEBUG, "sending to peer on fd %d", 
             peer.GetFD());
        try
        {
            peer.SendPDU(pdu);
        }
        catch (EPeerSendFailed &e)
        {
            hlog(HLOG_WARN, "%s", e.what().c_str());
        }
    }
}
// void Forte::PDUPeerSet::UpdatePollFDs(struct pollfd [] &pollFDs, int &nfds)
// {
//     nfds = mFDMap.size();
//     int i = 0;
//     foreach (const FDPair &p, mFDMap)
//     {
//         PDUPeer &peer(*(p.second));
//         pollFDs[i].fd = peer.getFD();
//         pollFDs[i].events = 
//             }
// }

