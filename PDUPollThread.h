// #SCQAD TAG: forte.pdupeer
#ifndef __Forte_PDUPollThread_h_
#define __Forte_PDUPollThread_h_

#include <boost/make_shared.hpp>
#include "Event.h"
#include "ThreadPoolDispatcher.h"

namespace Forte
{
    class PDUPollThread : public Thread
    {
    public:
        PDUPollThread(PDUPeerSet& peerSet)
            : mPDUPeerSet(peerSet)
            {
                initialized();
            }

        virtual ~PDUPollThread() {
            deleting();
        }

    protected:
        void* run() {
            FTRACE;
            hlog(HLOG_DEBUG, "Starting poller thread");
            while (!Thread::MyThread()->IsShuttingDown())
            {
                try
                {
                    mPDUPeerSet.Poll(100);
                }
                catch (EPDUPeerSetNotPolling& e)
                {
                    //TODO: make this more responsive or ensure it
                    // doesn't happen. maybe re-enqueue the
                    // PDUPollThread and exit? happens during
                    // start up and shutdown. for now, wait a second.
                    if (hlog_ratelimit(20))
                        hlog(HLOG_DEBUG2, "PeerSetNot Polling");
                    InterruptibleSleep(Timespec::FromSeconds(1));
                }
                catch (EThreadPoolDispatcherShuttingDown& e)
                {
                }
                catch (Exception &e)
                {
                    hlog(HLOG_ERR,
                         "unexpected exception while polling PDUPeerSet: %s",
                         e.what());
                    InterruptibleSleep(Timespec::FromSeconds(1));
                }
                catch (...)
                {
                    if (hlog_ratelimit(60))
                        hlog(HLOG_ERR, "Unknown exception while polling");
                    InterruptibleSleep(Timespec::FromSeconds(1));
                }
            }
            return NULL;
        }

    protected:
        PDUPeerSet& mPDUPeerSet;
    };
    typedef boost::shared_ptr<PDUPollThread> PDUPollThreadPtr;
};

#endif
