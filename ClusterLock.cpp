// ClusterLock.cpp
#include "ClusterLock.h"
#include "FTrace.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// constants
const char* Forte::CClusterLock::LOCK_PATH = "/var/lock";
const unsigned Forte::CClusterLock::DEFAULT_TIMEOUT = 120;
const int Forte::CClusterLock::TIMER_SIGNAL = SIGRTMIN + 10;
const char* Forte::CClusterLock::VALID_LOCK_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "/-_=+.@#%^?";


// statics
Forte::Mutex Forte::CClusterLock::sMutex;
std::map<Forte::FString, Forte::Mutex> Forte::CClusterLock::sMutexMap;
bool Forte::CClusterLock::sSigactionInitialized = false;

// debug statics
// CMutex CClusterLock::s_counter_mutex;
// int CClusterLock::s_counter = 0;
// int CClusterLock::s_outstanding_locks = 0;
// int CClusterLock::s_outstanding_timers = 0;

using namespace Forte;
// -----------------------------------------------------------------------------

// ctor/dtor
CClusterLock::CClusterLock(const FString& name, unsigned timeout)
    : mName(name)
    , mFD(-1)
    , mTimer()
    , mMutex(NULL)
{
    init();
    lock(name, timeout);
}


CClusterLock::CClusterLock()
    : mFD(-1)
    , mTimer()
    , mMutex(NULL)
{
    init();
}


CClusterLock::~CClusterLock()
{
    unlock();
    fini();
}


// helpers
void CClusterLock::sig_action(int sig, siginfo_t *info, void *context)
{
    // cout << pthread_self() << "  caught signal " << sig << endl;
}


void CClusterLock::init()
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
            sa.sa_sigaction = &CClusterLock::sig_action;
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
//         hlog(HLOG_DEBUG4, "CClusterLock: %u ( timer create, %i outstanding", m_counter, s_outstanding_timers);
//     }
}


void CClusterLock::fini()
{
    /*int ret_code;
    ret_code = timer_delete(mTimer);
    if (ret_code != 0)
    {
        // not sure if we can do anything about this but it'd be nice to know
        hlog(HLOG_ERR, "CClusterLock: Failed to delete timer. %i", ret_code);
        }*/
    
//     //debug
//     {
//         CAutoUnlockMutex lock(s_counter_mutex);
//         s_outstanding_timers--;
//         hlog(HLOG_DEBUG4, "CClusterLock: %u ) timer delete, %i outstanding", m_counter, s_outstanding_timers);
//     }
}


// lock/unlock
void CClusterLock::lock(const FString& name, unsigned timeout, const FString& errorString)
{
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
        hlog(HLOG_INFO, "CClusterLock - reusing lock with a different name");
        unlock();
    }

    // assign mutex
    {
        AutoUnlockMutex lock(sMutex);
        mMutex = &sMutexMap[filename];
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

    //hlog(HLOG_DEBUG4, "CClusterLock: %u [ locked", mTimer.TimerID());
//     //debug
//     {
//         CAutoUnlockMutex lock(s_counter_mutex);
//         s_outstanding_locks++;
//         hlog(HLOG_DEBUG4, "CClusterLock: %u [ locked, %i outstanding", m_counter, s_outstanding_locks);
//     }
}


void CClusterLock::unlock()
{
    if (mName == "/fsscale0/lock/state_db")
        hlog(HLOG_DEBUG, "UNLOCKED STATE DB");

    // clear name
    mName.clear();

    // locked?
    if (mFD != -1)
    {
        // release lock
        mLock->unlock();
        mLock.reset();
        mFD.Close();
        //hlog(HLOG_DEBUG4, "CClusterLock: %u ] unlocked", mTimer.TimerID());
//         //debug
//         {
//             CAutoUnlockMutex lock(s_counter_mutex);
//             s_outstanding_locks--;
//             hlog(HLOG_DEBUG4, "CClusterLock: %u ] unlocked, %i outstanding", m_counter, s_outstanding_locks);
//         }
    }
//     //debug
//     else
//     {
//         hlog(HLOG_DEBUG4, "CClusterLock: unlock called on %i mFD is -1", m_counter);
//     }

    // release mutex?
    if (mMutex != NULL)
    {
        mMutex->Unlock();
        mMutex = NULL;
    }
}
