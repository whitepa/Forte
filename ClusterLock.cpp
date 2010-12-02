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
std::map<Forte::FString, Forte::Mutex> Forte::ClusterLock::sMutexMap;
std::map<Forte::FString, boost::shared_ptr<Forte::ThreadKey> > Forte::ClusterLock::sThreadKeyMap;
bool Forte::ClusterLock::sSigactionInitialized = false;

// debug statics
// CMutex ClusterLock::s_counter_mutex;
// int ClusterLock::s_counter = 0;
// int ClusterLock::s_outstanding_locks = 0;
// int ClusterLock::s_outstanding_timers = 0;

using namespace Forte;
using namespace boost;
// -----------------------------------------------------------------------------

// ctor/dtor
ClusterLock::ClusterLock(const FString& name, unsigned timeout)
    : mName(name)
    , mFD(-1)
    , mTimer()
    , mMutex(NULL)
{
    init();
    lock(name, timeout);
}

ClusterLock::ClusterLock(const FString& name, unsigned timeout,  
                         const Forte::FString& errorString)
    : mName(name)
    , mFD(-1)
    , mTimer()
    , mMutex(NULL)
{
    init();
    lock(name, timeout, errorString);
}


ClusterLock::ClusterLock()
    : mFD(-1)
    , mTimer()
    , mMutex(NULL)
{
    init();
}


ClusterLock::~ClusterLock()
{
    unlock();
    fini();
}


// helpers
void ClusterLock::sig_action(int sig, siginfo_t *info, void *context)
{
    // cout << pthread_self() << "  caught signal " << sig << endl;
}


void ClusterLock::init()
{
//     //debug
//     {
//         CAutoUnlockMutex lock(s_counter_mutex);
//         s_counter++;
//         m_counter = s_counter;
//     }

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
                                     e.what().c_str());
    }

//     //debug
//     {
//         CAutoUnlockMutex lock(s_counter_mutex);
//         s_outstanding_timers++;
//         hlog(HLOG_DEBUG4, "ClusterLock: %u ( timer create, %i outstanding", m_counter, s_outstanding_timers);
//     }
}


void ClusterLock::fini()
{
    /*int ret_code;
    ret_code = timer_delete(mTimer);
    if (ret_code != 0)
    {
        // not sure if we can do anything about this but it'd be nice to know
        hlog(HLOG_ERR, "ClusterLock: Failed to delete timer. %i", ret_code);
        }*/
    
//     //debug
//     {
//         CAutoUnlockMutex lock(s_counter_mutex);
//         s_outstanding_timers--;
//         hlog(HLOG_DEBUG4, "ClusterLock: %u ) timer delete, %i outstanding", m_counter, s_outstanding_timers);
//     }
}


// lock/unlock
void ClusterLock::lock(const FString& name, unsigned timeout, const FString& errorString)
{
    FTRACE2("name='%s' timeout=%u", name.c_str(), timeout);
    if (name == "/fsscale0/lock/state_db")
        hlog(HLOG_DEBUG, "LOCK STATE DB");

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
        unlock();
    }
    mName = filename;

    // assign mutex and thread key
    {
        AutoUnlockMutex lock(sMutex);
        shared_ptr<ThreadKey> key;
        mMutex = &sMutexMap[mName];
        if (sThreadKeyMap.find(mName) == sThreadKeyMap.end())
        {
            key = sThreadKeyMap[mName] = make_shared<ThreadKey>();
            key->Set(0);
        }
        else {
            key = sThreadKeyMap[mName];
        }

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
        if (!(locked = mLock->exclusiveLock(true)))
        {
            // release mutex
            mMutex->Unlock();
            mMutex = NULL;
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
        mMutex = NULL;
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

    if (name == "/fsscale0/lock/state_db")
        hlog(HLOG_DEBUG, "LOCKED STATE DB");

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


    //hlog(HLOG_DEBUG4, "ClusterLock: %u [ locked", mTimer.TimerID());
//     //debug
//     {
//         CAutoUnlockMutex lock(s_counter_mutex);
//         s_outstanding_locks++;
//         hlog(HLOG_DEBUG4, "ClusterLock: %u [ locked, %i outstanding", m_counter, s_outstanding_locks);
//     }
}


void ClusterLock::unlock()
{
    FTRACE2("name='%s'", mName.c_str());
    if (mName == "/fsscale0/lock/state_db")
        hlog(HLOG_DEBUG, "UNLOCKED STATE DB");

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
        }
    }

    // clear name
    mName.clear();

    // locked?
    if (mFD != -1)
    {
        // release lock
        mLock->unlock();
        mLock.reset();
        mFD.Close();
        //hlog(HLOG_DEBUG4, "ClusterLock: %u ] unlocked", mTimer.TimerID());
//         //debug
//         {
//             CAutoUnlockMutex lock(s_counter_mutex);
//             s_outstanding_locks--;
//             hlog(HLOG_DEBUG4, "ClusterLock: %u ] unlocked, %i outstanding", m_counter, s_outstanding_locks);
//         }
    }
//     //debug
//     else
//     {
//         hlog(HLOG_DEBUG4, "ClusterLock: unlock called on %i mFD is -1", m_counter);
//     }

    // release mutex?
    if (mMutex != NULL)
    {
        mMutex->Unlock();
        mMutex = NULL;
    }
}
