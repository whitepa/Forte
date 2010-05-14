// ClusterLock.cpp
#include "Forte.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// constants
const char *ClusterLock::LOCK_PATH = "/var/lock";
const unsigned ClusterLock::DEFAULT_TIMEOUT = 120;
const int ClusterLock::TIMER_SIGNAL = SIGRTMIN + 10;
const char *ClusterLock::VALID_LOCK_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "/-_=+.@#%^?";


// statics
Mutex ClusterLock::sMutex;
std::map<FString, Mutex> ClusterLock::sMutexMap;
bool ClusterLock::sSigactionInitialized = false;

// ctor/dtor
ClusterLock::ClusterLock(const FString& name, unsigned timeout, const FString& errorString)
    : mTimer()
{
    init();
    Lock(name, timeout);
}


ClusterLock::ClusterLock()
    : mTimer()
{
    init();
}


ClusterLock::~ClusterLock()
{
    Unlock();
    fini();
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
    mMutex = NULL;

    // create lock path
    //TODO: get this from the application context
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
    catch (ETimer& e)
    {
        // translate error to what we are expected to throw
        throw EClusterLockUnvailable(FStringFC(),
                                     "LOCK_TIMER_FAIL|||%s", 
                                     e.What().c_str());
    }

}


void ClusterLock::fini()
{
}


// lock/unlock
void ClusterLock::Lock(const FString& name, unsigned timeout, const FString& errorString)
{
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
        throw EClusterLock("LOCK_INVALID_NAME|||" + name);
    }

    // prepend default path?
    if (name.Left(1) != "/") filename = LOCK_PATH + FString("/") + name;

    // changing lock names?
    if (filename != mName && (!mName.empty()))
    {
        hlog(HLOG_INFO, "CClusterLock - reusing lock with a different name");
        Unlock();
    }

    // assign mutex
    {
        AutoUnlockMutex lock(sMutex);
        mMutex = &sMutexMap[filename];
    }

    // open file?
    if (mFD == -1)
    {
        mName = filename;
        mFD = open(filename, O_RDWR | O_CREAT | O_NOATIME, 0600);

        if (mFD == -1)
        {
            err = errno;

            hlog(HLOG_DEBUG, "could not open lock file: %s", 
                 mFileSystem.StrError(err).c_str());

            throw EClusterLockFile(
                errorString.empty() ? "LOCK_FAIL|||" + filename 
                + "|||" + mFileSystem.StrError(err) : 
                errorString);
        }

        ftruncate(mFD, 1);
        mLock.reset(new AdvisoryLock(mFD, 0, 1));
    }

    // start timer
    memset(&ts, 0, sizeof(ts));
    ts.it_value.tv_sec = timeout;
    timer_settime(mTimer.TimerID(), 0, &ts, NULL);

    // acquire mutex?
    if ((err = mMutex->TimedLock(ts.it_value)) == 0)
    {
        // acquire lock?
        if (!(locked = mLock->ExclusiveLock(true)))
        {
            // release mutex
            mMutex->Unlock();
            mMutex = NULL;
        }

        // did the lock timeout?
        timer_gettime(mTimer.TimerID(), &ts);
        timed_out = (ts.it_value.tv_sec == 0 && ts.it_value.tv_nsec == 0);
    }
    else
    {
        // set flags
        locked = false;
        timed_out = (err == ETIMEDOUT);
        mMutex = NULL;
        cout << pthread_self() << "  err = " << err << endl;
    }

    // stop timer
    memset(&ts, 0, sizeof(ts));
    timer_settime(mTimer.TimerID(), 0, &ts, NULL);

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
}


void ClusterLock::Unlock()
{
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
    if (mMutex != NULL)
    {
        mMutex->Unlock();
        mMutex = NULL;
    }
}



//////////////// Advisory Locking ////////////////


ClusterLock::AdvisoryLock::AdvisoryLock(int fd, off64_t start, off64_t len, short whence)
{
    mFD = fd;
    mLock.l_whence = whence;
    mLock.l_start = start;
    mLock.l_len = len;
}

/// getLock gets the first lock that blocks the lock description
///
ClusterLock::AdvisoryLock ClusterLock::AdvisoryLock::GetLock(bool exclusive)
{
    AdvisoryLock lock(*this);
    if (exclusive)
        lock.mLock.l_type = F_WRLCK;
    else
        lock.mLock.l_type = F_RDLCK;
    fcntl(mFD, F_GETLK, &lock);
    return lock;
}

/// sharedLock will return true on success, false if the lock failed
///
bool ClusterLock::AdvisoryLock::SharedLock(bool wait)
{
    mLock.l_type = F_RDLCK;
    if (fcntl(mFD, wait ? F_SETLKW : F_SETLK, &mLock) == -1)
        return false;
    return true;
}

/// exclusiveLock will return true on success, false if the lock failed
///
bool ClusterLock::AdvisoryLock::ExclusiveLock(bool wait)
{
    mLock.l_type = F_WRLCK;
    if (fcntl(mFD, wait ? F_SETLKW : F_SETLK, &mLock) == -1)
        return false;
    return true;
}

/// unlock will remove the current lock
///
void ClusterLock::AdvisoryLock::Unlock(void)
{
    mLock.l_type = F_UNLCK;
    fcntl(mFD, F_SETLK, &mLock);
}
