#include "DaemonUtil.h"
#include "MockNetworkConnection.h"
#include <sys/wait.h>

using namespace Forte;

MockNetworkConnection::MockNetworkConnection(
    const MockNetworkConnection::MockNetworkNodePtr& nodeWhoseStateIsSaved,
    const MockNetworkConnection::MockNetworkNodePtr& nodeWhoseStateIsLost) :
    mNodeWhoseStateIsSaved (nodeWhoseStateIsSaved),
    mNodeWhoseStateIsLost (nodeWhoseStateIsLost)
{
}

MockNetworkConnection::~MockNetworkConnection()
{
}

void MockNetworkConnection::RunMockNetwork()
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        throw EUnableToCreateSocketPair();
    int parentfd = fds[0];
    int childfd = fds[1];

    pid_t childPid = DaemonUtil::ForkSafely();
    if (childPid < 0)
    {
        close(parentfd);
        close(childfd);
        throw EUnableToForkNetworkNode();
    }
    else if (childPid == 0)
    {
        // child
        // NOTE: the network node whose state is not to
        // be saved is run on the child process.
        
        // first close the parent fd
        close(parentfd);
        
        // now run the network node
        mNodeWhoseStateIsLost->runAndCloseFD(childfd, true);
    }
    else
    {
        // parent
        
        // first close the child fd
        close(childfd);
        
        // now run the network node
        mNodeWhoseStateIsSaved->runAndCloseFD(parentfd, false);

        // wait for all child pids to finish
        int status;
        ::wait(&status);
    }
}
