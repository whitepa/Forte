#include "PDUPeerSetWorkHandler.h"
#include "FTrace.h"

using namespace Forte;
using namespace boost;

PDUPeerSetWorkHandler::PDUPeerSetWorkHandler() :
    RequestHandler(0)
{
    FTRACE;
}

PDUPeerSetWorkHandler::~PDUPeerSetWorkHandler()
{
    FTRACE;
}

void PDUPeerSetWorkHandler::Handler(Forte::Event* e)
{
    FTRACE;

    PDUEvent* event = dynamic_cast<PDUEvent*>(e);
    if (event == NULL)
    {
        throw std::bad_cast();
    }

    event->DoWork();
}
