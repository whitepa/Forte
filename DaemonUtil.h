#ifndef __forte_daemon_util_h_
#define __forte_daemon_util_h_

#include "Object.h"
#include "Exception.h"

namespace Forte
{
    EXCEPTION_CLASS(EDaemonUtil);
    EXCEPTION_SUBCLASS2(EDaemonUtil, EDaemonForkFailedException, "fork() failed");

    class DaemonUtil : public Object
    {
    public:
        /// Daemonize will correctly place your process in the background,
        /// disconnecting from the controlling terminal, changing the
        /// current directory to "/", detaching from the parent process,
        /// and redirecting stdin, stdout, stderr to /dev/null.  NOTE:
        /// this does not duplicate threads which have already been
        /// created, therefore this should be called as early as possible.
        /// If threads already exist in the calling process, this is
        /// fairly dangerous.  If you aren't sure what you are doing, ask
        /// someone who is.
        static void Daemonize(void);

        /// ForkDaemon() is similar to Daemonize, except the call will
        /// return in *both* processes.  This is useful if the parent
        /// process needs to perform some operation immediately after the
        /// Daemonize() call, prior to exiting.
        ///
        /// Returns false in the parent (original) process, and true in
        /// the new daemon process.
        static bool ForkDaemon(void);
    };
};
#endif
