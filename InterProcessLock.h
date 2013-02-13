#ifndef __forte__InterProcessLock_h
#define __forte__InterProcessLock_h

#include "Types.h"
#include "AdvisoryLock.h"
#include "AutoFD.h"
#include "FileSystemImpl.h"
#include "PosixTimer.h"
#include <csignal>
#include <ctime>
#include "boost/shared_ptr.hpp"

namespace Forte{

    EXCEPTION_CLASS(EInterProcessLock);

    EXCEPTION_SUBCLASS(EInterProcessLock,
                       EInterProcessLockTimeout);

    EXCEPTION_SUBCLASS(EInterProcessLock,
                       EInterProcessLockUnvailable);

    EXCEPTION_SUBCLASS(EInterProcessLock,
                       EInterProcessLockFile);
// -----------------------------------------------------------------------------

    class InterProcessLock
    {
    public:
        InterProcessLock();
        InterProcessLock(const Forte::FString& name, unsigned timeout = 120);
        InterProcessLock(const Forte::FString& name, unsigned timeout, const Forte::FString& errorString);
        virtual ~InterProcessLock();

        void Unlock();
        void Lock(const Forte::FString& name, unsigned timeout, const Forte::FString& errorString = "");

        inline Forte::FString GetName() const { return mName; }

        /**
         * NumLocks indicates how many cluster locks in this process are
         * in a locked state.
         *
         * @return unsigned integer number of locks held
         */
        static unsigned int NumLocks(void) { AutoUnlockMutex lock(sMutex); return sMutexMap.size(); }


        // constants
        static const char *LOCK_PATH;
        static const unsigned DEFAULT_TIMEOUT;
        static const int TIMER_SIGNAL;
        static const char *VALID_LOCK_CHARS;

    protected:
        Forte::FString mName;
        Forte::AutoFD mFD;
        std::auto_ptr<Forte::AdvisoryLock> mLock;
        Forte::PosixTimer mTimer;
        boost::shared_ptr<Forte::Mutex> mMutex;

        Forte::FileSystemImpl mFileSystem;

        // helpers
        void init();
        void checkAndClearFromMutexMap(const Forte::FString& name);
        static void sig_action(int sig, siginfo_t *info, void *context);

        // statics
        static std::map<Forte::FString, boost::shared_ptr<Forte::Mutex> > sMutexMap;
        static Forte::Mutex sMutex;
        static bool sSigactionInitialized;
    };

}; /* namespace Forte{} */
// -----------------------------------------------------------------------------

#endif
