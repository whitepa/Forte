#ifndef __forte__cluster_lock_h
#define __forte__cluster_lock_h

#include "Types.h"
#include "AutoMutex.h"
#include "FileSystem.h"
#include <csignal>
#include <ctime>

namespace Forte
{

    EXCEPTION_SUBCLASS(Exception, EClusterLock);

    EXCEPTION_SUBCLASS(EClusterLock, 
                       EClusterLockTimeout);

    EXCEPTION_SUBCLASS(EClusterLock,
                       EClusterLockUnvailable);

    EXCEPTION_SUBCLASS(EClusterLock,
                       EClusterLockFile);

    class ClusterLock : public Object
    {
    public:
        ClusterLock();
        ClusterLock(const FString& name, unsigned timeout = 120, const FString& errorString = "");
        virtual ~ClusterLock();

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
        Mutex *m_mutex;

        // helpers
        void init();
        void fini();
        static void sig_action(int sig, siginfo_t *info, void *context);

        // statics
        static Mutex s_mutex;
        static std::map<FString, Mutex> s_mutex_map;
        static bool s_sigactionInitialized;
    };
};
#endif
