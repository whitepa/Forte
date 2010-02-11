// ClusterLock.cpp
#include "Forte.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// constants
const char *CClusterLock::LOCK_PATH = "/var/lock";
const unsigned CClusterLock::DEFAULT_TIMEOUT = 120;
const int CClusterLock::TIMER_SIGNAL = SIGRTMIN + 10;
const char *CClusterLock::VALID_LOCK_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "/-_=+.@#%^?";


// statics
CMutex CClusterLock::s_mutex;
std::map<FString, CMutex> CClusterLock::s_mutex_map;
bool CClusterLock::s_sigactionInitialized = false;

// ctor/dtor
CClusterLock::CClusterLock(const FString& name, unsigned timeout, const FString& errorString)
{
    init();
    lock(name, timeout);
}


CClusterLock::CClusterLock()
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
    struct sigaction sa;
    sigevent_t se;
    int err;

    // init members
    m_mutex = NULL;

    // create lock path
    FileSystem::get()->mkdir(LOCK_PATH, 0777, true);

    {
        CAutoUnlockMutex lock(s_mutex);
        if (!s_sigactionInitialized)
        {
            // init sigaction (once per process)
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_sigaction = &CClusterLock::sig_action;
            sa.sa_flags = SA_SIGINFO;
            sigaction(TIMER_SIGNAL, &sa, NULL);
            s_sigactionInitialized = true;
        }
    }
    // init sigevent
    memset(&se, 0, sizeof(se));
    se.sigev_value.sival_ptr = this;
    se.sigev_signo = TIMER_SIGNAL;
    se.sigev_notify = SIGEV_SIGNAL | SIGEV_THREAD_ID;
    se._sigev_un._tid = (long int)syscall(SYS_gettid);

    // create timer
    int tries = 0;
    while ((err = timer_create(CLOCK_MONOTONIC, &se, &m_timer)) != 0)
    {
        if (err == -EAGAIN)
        {
            if (++tries < 100)
            {
                hlog(HLOG_DEBUG, "got EAGAIN from timer_create");
                usleep(100000); // TODO: should we subtract from the timeout?
                continue;                
            }
            else
            {
                err = errno;
                throw EClusterLockUnvailable("LOCK_TIMER_FAIL|||" +
                                             FileSystem::get()->strerror(err));
            }
        }
        err = errno;

        throw EClusterLock("LOCK_TIMER_FAIL|||" +
                           FileSystem::get()->strerror(err));
    }
}


void CClusterLock::fini()
{
    timer_delete(m_timer);
}


// lock/unlock
void CClusterLock::lock(const FString& name, unsigned timeout, const FString& errorString)
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

    // prepend default path?
    if (name.Left(1) != "/") filename = LOCK_PATH + FString("/") + name;

    // changing lock names?
    if (filename != m_name) unlock();

    // assign mutex
    {
        CAutoUnlockMutex lock(s_mutex);
        m_mutex = &s_mutex_map[filename];
    }

    // open file?
    if (m_fd == -1)
    {
        m_name = filename;
        m_fd = open(filename, O_RDWR | O_CREAT | O_NOATIME, 0600);

        if (m_fd == -1)
        {
            err = errno;

            hlog(HLOG_DEBUG, "could not open lock file: %s", 
                 FileSystem::get()->strerror(err).c_str());

            throw EClusterLockFile(
                errorString.empty() ? "LOCK_FAIL|||" + filename 
                + "|||" + FileSystem::get()->strerror(err) : 
                errorString);
        }

        ftruncate(m_fd, 1);
        m_lock.reset(new FileSystem::AdvisoryLock(m_fd, 0, 1));
    }

    // start timer
    memset(&ts, 0, sizeof(ts));
    ts.it_value.tv_sec = timeout;
    timer_settime(m_timer, 0, &ts, NULL);

    // acquire mutex?
    if ((err = m_mutex->timedlock(ts.it_value)) == 0)
    {
        // acquire lock?
        if (!(locked = m_lock->exclusiveLock(true)))
        {
            // release mutex
            m_mutex->unlock();
            m_mutex = NULL;
        }

        // did the lock timeout?
        timer_gettime(m_timer, &ts);
        timed_out = (ts.it_value.tv_sec == 0 && ts.it_value.tv_nsec == 0);
    }
    else
    {
        // set flags
        locked = false;
        timed_out = (err == ETIMEDOUT);
        m_mutex = NULL;
        cout << pthread_self() << "  err = " << err << endl;
    }

    // stop timer
    memset(&ts, 0, sizeof(ts));
    timer_settime(m_timer, 0, &ts, NULL);

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


void CClusterLock::unlock()
{
    // clear name
    m_name.clear();

    // locked?
    if (m_fd != -1)
    {
        // release lock
        m_lock->unlock();
        m_lock.reset();
        m_fd.close();
    }

    // release mutex?
    if (m_mutex != NULL)
    {
        m_mutex->unlock();
        m_mutex = NULL;
    }
}

