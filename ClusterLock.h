#ifndef __forte__cluster_lock_h
#define __forte__cluster_lock_h

#include "Types.h"
#include "AutoMutex.h"
#include "FileSystem.h"
#include "Timer.h"
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
        // helper classes
        class AdvisoryLock
        {
        public:
            AdvisoryLock(int fd, off64_t start, off64_t len, short whence = SEEK_SET);
        
            /// getLock returns a lock description equivalent to the lock
            /// currently blocking us.
            AdvisoryLock GetLock(bool exclusive = false);

            /// sharedLock will return true on success, false if the lock failed
            ///
            bool SharedLock(bool wait = true);
        
            /// exclusiveLock will return true on success, false if the lock failed
            ///
            bool ExclusiveLock(bool wait = true);

            /// unlock will remove the current lock
            ///
            void Unlock(void);

         protected:
            struct flock mLock;
            int mFD;
        };

        class AdvisoryAutoUnlock
        {
        public:
        AdvisoryAutoUnlock(int fd, off64_t start, off64_t len, bool exclusive, 
                           short whence = SEEK_SET) :
            mAdvisoryLock(fd, start, len, whence)
            {
                if (exclusive)
                    mAdvisoryLock.ExclusiveLock();
                else
                    mAdvisoryLock.SharedLock();
                }
            virtual ~AdvisoryAutoUnlock() { mAdvisoryLock.Unlock(); }
        protected:
            AdvisoryLock mAdvisoryLock;
        };

    public:
        ClusterLock();
        ClusterLock(const FString& name, unsigned timeout = 120, const FString& errorString = "");
        virtual ~ClusterLock();

        void Unlock();
        void Lock(const FString& name, unsigned timeout, const FString& errorString = "");

        inline FString GetName() const { return mName; }
    
        // constants
        static const char *LOCK_PATH;
        static const unsigned DEFAULT_TIMEOUT;
        static const int TIMER_SIGNAL;
        static const char *VALID_LOCK_CHARS;

     protected:
        //TODO: Pull this from the application context
        FileSystem mFileSystem;

        FString mName;
        AutoFD mFD;
        std::auto_ptr<AdvisoryLock> mLock;
        Timer mTimer;
        Mutex *mMutex;

        // helpers
        void init();
        void fini();
        static void sig_action(int sig, siginfo_t *info, void *context);

        // statics
        static Mutex sMutex;
        static std::map<FString, Mutex> sMutexMap;
        static bool sSigactionInitialized;
    };
};
#endif
