#ifndef __forte__cluster_lock_h
#define __forte__cluster_lock_h

#include "Types.h"
#include "AutoMutex.h"
#include "AdvisoryLock.h"
#include "AutoFD.h"
#include "FileSystem.h"
#include "PosixTimer.h"
#include <csignal>
#include <ctime>

namespace Forte{

EXCEPTION_CLASS(EClusterLock);

EXCEPTION_SUBCLASS(EClusterLock, 
                   EClusterLockTimeout);

EXCEPTION_SUBCLASS(EClusterLock,
                   EClusterLockUnvailable);

EXCEPTION_SUBCLASS(EClusterLock,
                   EClusterLockFile);
// -----------------------------------------------------------------------------

class ClusterLock
{
public:
    ClusterLock();
    ClusterLock(const Forte::FString& name, unsigned timeout = 120);
    ClusterLock(const Forte::FString& name, unsigned timeout, const Forte::FString& errorString);
    virtual ~ClusterLock();

    void unlock();
    void lock(const Forte::FString& name, unsigned timeout, const Forte::FString& errorString = "");

    inline Forte::FString getName() const { return mName; }
    
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
    Forte::Mutex* mMutex;

    Forte::FileSystem  mFileSystem;

    // helpers
    void init();
    void fini();
    static void sig_action(int sig, siginfo_t *info, void *context);

    // statics
    static std::map<Forte::FString, Forte::Mutex> sMutexMap;
    static Forte::Mutex sMutex;
    static bool sSigactionInitialized;

/*     // debugging  */
/*     static CMutex s_counter_mutex; */
/*     static int s_counter; */
/*     static int s_outstanding_locks; */
/*     static int s_outstanding_timers; */
/*     unsigned int m_counter; */
};

}; /* namespace Forte{} */
// -----------------------------------------------------------------------------

#endif
