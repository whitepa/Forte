#ifndef __forte__cluster_lock_h
#define __forte__cluster_lock_h

#include "Types.h"
#include "AutoMutex.h"
#include "FileSystem.h"
#include <csignal>
#include <ctime>

EXCEPTION_SUBCLASS(CForteException, EClusterLock);

EXCEPTION_SUBCLASS(EClusterLock, 
                   EClusterLockTimeout);

EXCEPTION_SUBCLASS(EClusterLock,
                   EClusterLockUnvailable);

EXCEPTION_SUBCLASS(EClusterLock,
                   EClusterLockFile);

class CClusterLock
{
public:
    CClusterLock();
    CClusterLock(const FString& name, unsigned timeout = 120, const FString& errorString = "");
    virtual ~CClusterLock();

    void unlock();
    void lock(const FString& name, unsigned timeout, const FString& errorString = "");

    inline FString getName() const { return m_name; }
    
    // constants
    static const char *LOCK_PATH;
    static const unsigned DEFAULT_TIMEOUT;
    static const int TIMER_SIGNAL;
    static const char *VALID_LOCK_CHARS;

protected:
    FString m_name;
    AutoFD m_fd;
    std::auto_ptr<FileSystem::AdvisoryLock> m_lock;
    timer_t m_timer;
    CMutex *m_mutex;

    // helpers
    void init();
    void fini();
    static void sig_action(int sig, siginfo_t *info, void *context);

    // statics
    static CMutex s_mutex;
    static std::map<FString, CMutex> s_mutex_map;
    static bool s_sigactionInitialized;
};

#endif
