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

    // init members
    mMutex.reset();

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

void ClusterLock::checkAndClearFromMutexMap(const Forte::FString& name)
{
    AutoUnlockMutex lock(sMutex);
    std::map<FString, shared_ptr<Mutex> >::const_iterator i =
        sMutexMap.find(name);
    if (i == sMutexMap.end())
        hlog(HLOG_ERR, "could not find mutex while unlocking '%s'",
             name.c_str());
    else
    {
        const shared_ptr<Mutex> &mp(i->second);
        if (mp.unique())
        {
            hlog(HLOG_DEBUG4, "lock %s no longer in use; freeing mutex",
                 name.c_str());
            sMutexMap.erase(name);
        }
        else
        {
            hlog(HLOG_DEBUG4, "lock %s still in use; will not free mutex",
                 name.c_str());                
        }
    }    
}

// lock/unlock
void ClusterLock::Lock(const FString& name, unsigned timeout, const FString& errorString)
{
    FTRACE2("name='%s' timeout=%u", name.c_str(), timeout);

    struct itimerspec ts;
    bool locked, timed_out = false;
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

    // assign mutex
    {
        AutoUnlockMutex lock(sMutex);

        if (sMutexMap.find(mName) == sMutexMap.end())
        {
            sMutexMap[mName] = make_shared<Mutex>();
        }

        mMutex = sMutexMap[mName];

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
            // mName = filename;
            mFD = open(mName, O_RDWR | O_CREAT | O_NOATIME, 0600);
                
            if (mFD == -1)
            {
                err = errno;
                    
                hlog(HLOG_WARN, "could not open lock file: %s", 
                     strerror(errno));

                // release the mutex
                mMutex->Unlock();
                mMutex.reset();
                checkAndClearFromMutexMap(mName);
                    
                throw EClusterLockFile(
                    errorString.empty() ? "LOCK_FAIL|||" + mName
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
            mMutex.reset();
            checkAndClearFromMutexMap(mName);
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
                 strerror(errno));
        }

        mMutex.reset();
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
                throw EClusterLockTimeout("LOCK_TIMEOUT|||" + mName);
        }
        else
        {
            if (!errorString.empty())
                throw EClusterLock(errorString);
            else
                throw EClusterLock("LOCK_FAIL|||" + mName);
        }
    }
}


void ClusterLock::Unlock()
{
    FTRACE2("name='%s'", mName.c_str());

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
        mMutex.reset();
        checkAndClearFromMutexMap(mName);
    }

    mName.clear();
}
