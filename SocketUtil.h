#ifndef __Forte_SocketUtil_h_
#define __Forte_SocketUtil_h_

#include <utility>
#include <vector>
#include <string>
#include "Exception.h"
#include "Clock.h"

namespace Forte
{
    typedef std::pair<std::string, int> SocketAddress;
    typedef std::vector<std::pair<std::string, int> > SocketAddressVector;

    EXCEPTION_CLASS(ESocketUtil);
    EXCEPTION_SUBCLASS2(
        ESocketUtil,
        ECouldNotConvertIP,
        "Could not convert IP address");

    EXCEPTION_SUBCLASS2(
        ESocketUtil,
        ECouldNotBind,
        "Could not bind to address");

    EXCEPTION_SUBCLASS2(
        ESocketUtil,
        ECouldNotCreateSocket,
        "Could not create socket");

    EXCEPTION_SUBCLASS2(
        ESocketUtil,
        ECouldNotConnect,
        "Could not connect to peer");

    EXCEPTION_SUBCLASS(ESocketUtil, EFcntlFailed);
    EXCEPTION_SUBCLASS(ESocketUtil, ESelectFailed);

    class SocketUtil {
      public:
        enum {
            SOCKET_RDONLY = 0,
            SOCKET_WRONLY,
            SOCKET_RDWR
        };
    };

    int createSocket(int domain, int type, int protocol);
    int createInetStreamSocket();

    void bindToAddress(int fd, const SocketAddress& connectToAddress);
    void connectToAddress(int fd, const SocketAddress& connectToAddress);

    void setTCPKeepAlive(int fd,
                         const int soKeepAliveEnabled=1,
                         const int tcpKeepAliveCount=4,
                         const int tcpKeepAliveIntervalSeconds = 10,
                         const int tcpKeepAliveIdleSeconds = 1);

    void setTCPUserTimeout(int fd,
                           const int tcpUserTimeoutInMillisec=30*1000);

    void setReuseAddr(int fd,
                      const int soResuseAddr=1);

    //works for not sockets too
    void setSocketNonBlocking(int fd);
    void setSocketNonBlockingSaveOldFlags(int fd, int &oldFlags);
    void restoreSocketFlags(int fd, int flags);

    void connectNonBlocking(
        int fd,
        const struct sockaddr *addr,
        socklen_t addrlen,
        const Forte::Timespec &timeout);

    int getTCPSendBufferSize(int fd);
    void setTCPSendBufferSize(int fd, int tcpBufferSize);

    void setTCPNoDelay(int fd);
    void setTCPQuickAck(int fd);

    void waitForSocketAvailable(int fd, int type, Forte::Timespec timeout);
};
#endif
