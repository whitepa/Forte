// ClusterLock.cpp
#include "ClusterLock.h"
#include "FTrace.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <boost/make_shared.hpp>

// constants
const char* Forte::ClusterLock::LOCK_PATH = "/var/lock";
const unsigned Forte::ClusterLock::DEFAULT_TIMEOUT = 120;
const int Forte::ClusterLock::TIMER_SIGNAL = SIGRTMIN + 10;
const char* Forte::ClusterLock::VALID_LOCK_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "/-_=+.@#%^?";


// statics
Forte::Mutex Forte::ClusterLock::sMutex;
std::map<Forte::FString, boost::shared_ptr<Forte::Mutex> > Forte::ClusterLock::sMutexMap;
std::map<Forte::FString, boost::shared_ptr<Forte::ThreadKey> > Forte::ClusterLock::sThreadKeyMap;
bool Forte::ClusterLock::sSigactionInitialized = false;

using namespace Forte;
using namespace boost;
// -----------------------------------------------------------------------------

// ctor/dtor
ClusterLock::ClusterLock(const FString& name, unsigned timeout)
    : mName(name)
    , mFD(-1)
    , mTimer()
{
    init();
    Lock(name, timeout);
}

ClusterLock::ClusterLock(const FString& name, unsigned timeout,  
                         const Forte::FString& errorString)
    : mName(name)
    , mFD(-1)
    , mTimer()
{
    init();
    Lock(name, timeout, errorString);
}


ClusterLock::ClusterLock()
    : mFD(-1)
    , mTimer()
{
    init();
}


ClusterLock::~ClusterLock()
{
    Unlock();
}


// helpers
void ClusterLock::sig_action(int sig, siginfo_t *info, void *context)
{
    // cout << pthread_self() << "  caught signal " << sig << endl;
}


void ClusterLock::init()
{
    struct sigaction sa;
    sigevent_t se;

    // create lock path
    mFileSystem.MakeDir(LOCK_PATH, 0777, true);

    {
        AutoUnlockMutex lock(sMutex);
        if (!sSigactionInitialized)
        {
            // init sigaction (once per process)
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_sigaction = &ClusterLock::sig_action;
            sa.sa_flags = SA_SIGINFO;
            sigaction(TIMER_SIGNAL, &sa, NULL);
            sSigactionInitialized = true;
        }
    }

    // init sigevent
    memset(&se, 0, sizeof(se));
    se.sigev_value.sival_ptr = this;
    se.sigev_signo = TIMER_SIGNAL;
    se.sigev_notify = SIGEV_SIGNAL | SIGEV_THREAD_ID;
    se._sigev_un._tid = (long int)syscall(SYS_gettid);

    try
    {
        mTimer.Init(se);        
    }
    catch (EPosixTimerInit& e)
    {
        // translate error to what we are expected to throw
        throw EClusterLockUnvailable(FStringFC(),
                                     "LOCK_TIMER_FAIL|||%s", 
                                     e.what());
    }
}


// lock/unlock
void ClusterLock::Lock(const FString& name, unsigned timeout, const FString& errorString)
{
    FTRACE2("name='%s' timeout=%u", name.c_str(), timeout);

    struct itimerspec ts;
    bool locked, timed_out;
    FString filename(name);
    int err;

    // validate name
    if (name.find_first_not_of(VALID_LOCK_CHARS) != NOPOS)
    {
        throw EClusterLock("LOCK_INVALID_NAME|||" + name);
    }

    if (name.empty())
    {
        throw EClusterLock("LOCK_INVALID_NAME");
    }

    // prepend default path?
    if (name.Left(1) != "/") filename = LOCK_PATH + FString("/") + name;

    // changing lock names?
    if (filename != mName && (!mName.empty()))
    {
        hlog(HLOG_INFO, "ClusterLock - reusing lock with a different name");
        Unlock();
    }
    mName = filename;

    // assign mutex and thread key
    {
        AutoUnlockMutex lock(sMutex);
        shared_ptr<ThreadKey> key;
        if (sThreadKeyMap.find(mName) == sThreadKeyMap.end())
        {
            key = sThreadKeyMap[mName] = make_shared<ThreadKey>();
            key->Set(0);
        }
        else {
            key = sThreadKeyMap[mName];
        }

        hlog(HLOG_DEBUG, "sThreadKeyMap.size: %zu, lock: %s, threadkey use_count: %ld\n",
             sThreadKeyMap.size(), mName.c_str(), key.use_count()); // use_count() is for DEBUG *only*

        if (sMutexMap.find(mName) == sMutexMap.end())
        {
            sMutexMap[mName] = make_shared<Mutex>();
        }

        mMutex = sMutexMap[mName];

        hlog(HLOG_DEBUG, "sMutexMap.size: %zu, lock: %s, mutex use_count: %ld\n",
             sMutexMap.size(), mName.c_str(), mMutex.use_count()); // use_count() is for DEBUG *only*

        // check to see if we already hold this cluster lock in this thread
        u64 refcount = (u64)key->Get();
        if (refcount > 0)
        {
            key->Set((void*)(++refcount));
            hlog(HLOG_DEBUG2, "cluster lock '%s' threadkey %p refcount incremented to %llu",
                 mName.c_str(), key.get(), refcount);
            return;
        }
    }

    // open file?

    // start timer
    memset(&ts, 0, sizeof(ts));
    ts.it_value.tv_sec = timeout;
    timer_settime(mTimer.PosixTimerID(), 0, &ts, NULL);

    // acquire mutex?
    if ((err = mMutex->TimedLock(ts.it_value)) == 0)
    {
        if (mFD == -1)
        {
            mName = filename;
            mFD = open(filename, O_RDWR | O_CREAT | O_NOATIME, 0600);

            if (mFD == -1)
            {
                err = errno;

                hlog(HLOG_WARN, "could not open lock file: %s", 
                     mFileSystem.StrError(err).c_str());

                throw EClusterLockFile(
                    errorString.empty() ? "LOCK_FAIL|||" + filename 
                    + "|||" + mFileSystem.StrError(err) : 
                    errorString);
            }

            ftruncate(mFD, 1);
            mLock.reset(new AdvisoryLock(mFD, 0, 1));
        }

        // acquire lock?
        if (!(locked = mLock->ExclusiveLock(true)))
        {
            // release mutex
            mMutex->Unlock();
        }

        // did the lock timeout?
        timer_gettime(mTimer.PosixTimerID(), &ts);
        timed_out = (ts.it_value.tv_sec == 0 && ts.it_value.tv_nsec == 0);
    }
    else
    {
        // set flags
        locked = false;
        if (err == ETIMEDOUT)
        {
            timed_out = true;            
        }
        else
        {
            hlog(HLOG_WARN, "%u could not get lock on file, not timed out: %s", 
                 (unsigned int) pthread_self(),
                 mFileSystem.StrError(err).c_str());
        }
    }

    // stop timer
    memset(&ts, 0, sizeof(ts));
    timer_settime(mTimer.PosixTimerID(), 0, &ts, NULL);

    // got the lock?
    if (!locked)
    {
        if (timed_out)
        {
            if (!errorString.empty())
                throw EClusterLockTimeout(errorString);
            else
                throw EClusterLockTimeout("LOCK_TIMEOUT|||" + filename);
        }
        else
        {
            if (!errorString.empty())
                throw EClusterLock(errorString);
            else
                throw EClusterLock("LOCK_FAIL|||" + filename);
        }
    }

    // we got the lock, increment the refcount
    {
        AutoUnlockMutex lock(sMutex);
        if (sThreadKeyMap.find(mName) == sThreadKeyMap.end())
        {
            hlog(HLOG_ERR, "failed to locate thread key after lock for cluster lock '%s'",
                 mName.c_str());
        }
        else
        {
            shared_ptr<ThreadKey> key = sThreadKeyMap[mName];

            // check to see if we already hold this cluster lock in this thread
            u64 refcount = (u64)key->Get();
            if (refcount != 0)
                hlog(HLOG_ERR, "cluster lock '%s' threadkey refcount > 0 after lock acquired",
                     name.c_str());
            key->Set((void*)(++refcount)); // should be setting this to 1
            hlog(HLOG_DEBUG2, "cluster lock '%s' threadkey %p refcount %llu after lock",
                 mName.c_str(), key.get(), refcount);
            hlog(HLOG_DEBUG2, "threadkey %p shows %p", key.get(), key->Get());
        }
    }
}


void ClusterLock::Unlock()
{
    FTRACE2("name='%s'", mName.c_str());

    if (!mName.empty())
    {
        AutoUnlockMutex lock(sMutex);
        if (sThreadKeyMap.find(mName) == sThreadKeyMap.end())
        {
            hlog(HLOG_ERR, "unlock failed to locate thread key for cluster lock '%s'", mName.c_str());
        }
        else
        {
            shared_ptr<ThreadKey> key = sThreadKeyMap[mName];
            u64 refcount = (u64)key->Get();
            if (refcount == 0) {
                hlog(HLOG_ERR, "cluster lock '%s' threadkey %p refcount == 0 before unlock",
                     mName.c_str(), key.get());
            }
            else
            {
                key->Set((void*)--refcount);
                hlog(HLOG_DEBUG2, "cluster lock '%s' threadkey %p refcount decremented to %llu",
                     mName.c_str(), key.get(), refcount);
                if (refcount > 0)
                {
                    return;
                }
            }

            hlog(HLOG_DEBUG, "sThreadKeyMap.size: %zu, lock: %s, thread key use_count: %ld\n",
                 sThreadKeyMap.size(), mName.c_str(), key.use_count()); // use_count() is for DEBUG *only*
        }
    }

    hlog(HLOG_DEBUG, "sMutexMap.size: %zu, lock: %s, mutex use_count: %ld\n",
         sMutexMap.size(), mName.c_str(), mMutex.use_count()); // use_count() is for DEBUG *only*

    // clear name
    mName.clear();

    // locked?
    if (mFD != -1)
    {
        // release lock
        mLock->Unlock();
        mLock.reset();
        mFD.Close();
    }

    // release mutex?
    if (mMutex)
    {
        mMutex->Unlock();
    }
}
