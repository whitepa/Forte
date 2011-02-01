#include "Clock.h"
#include "EventQueue.h"
#include "FileSystem.h"
#include "FTrace.h"
#include "LogManager.h"
#include "ProcRunner.h"
#include "Thread.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ClusterLock.h"

#include <boost/make_shared.hpp>

using namespace Forte;
using namespace boost;

LogManager logManager;

class EVerifyProcLocksEntry {};
void verifyFileHasEntryInProcLocks(const FString& theFile);
// -----------------------------------------------------------------------------

class ClusterLockTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() {
        logManager.BeginLogging("//stdout", HLOG_ALL);

        FileSystem fs;

        if (fs.FileExists("/fsscale0/lock"))
        {
            ClusterLock::LOCK_PATH = "/fsscale0/lock";
        }
        else
        {
            ClusterLock::LOCK_PATH = "./lock";
            fs.MakeDir("./lock");
        }       
    }

    static void TearDownTestCase() {

        if (ClusterLock::LOCK_PATH == "./lock")
        {
            FileSystem fs;
            fs.Unlink("./lock", true);
        }
    }
};
// -----------------------------------------------------------------------------

void verifyFileHasEntryInProcLocks(const FString& theFile)
{
    ProcRunner pr;
    FString output;
    FStringVector lines, entries, pieces;

    struct stat st;
    stat(theFile, &st);
        
    //1: POSIX  ADVISORY  WRITE 3725 fd:00:7758628 0 0
    int PID_ENTRY = 6;
    int FILEID_ENTRY = 7;
    int FILEID_INODE_PIECE = 2;

    FString cmd("/bin/cat /proc/locks");

    if (pr.Run(cmd, "", &output, PROC_RUNNER_DEFAULT_TIMEOUT) == 0)
    {
        hlog(HLOG_INFO, "/proc/locks: %s", output.c_str());
        output.Trim().LineSplit(lines, true);

        foreach (const FString& line, lines)
        {
            if (line == "")
            {
                continue;
            }
            hlog(HLOG_INFO, "lock: %s", line.c_str());
            line.Explode(" ", entries, true);

            hlog(HLOG_INFO, "pid entry: %s", entries[PID_ENTRY].c_str());
            hlog(HLOG_INFO, "fileid entry: %s", entries[FILEID_ENTRY].c_str());

            if (entries[PID_ENTRY].AsInteger() == getpid())
            {
                //maj-dev:min-dev:inode
                //fd:00:7758628
                entries[FILEID_ENTRY].Explode(":", pieces, true);

                hlog(HLOG_INFO,
                     "inode piece: %s",
                     pieces[FILEID_INODE_PIECE].c_str());

                if (pieces[FILEID_INODE_PIECE].AsUnsignedInteger() == st.st_ino)
                {
                    return;
                }
            }
        }
    }
    else
    {
        hlog(HLOG_ERR, "Could not cat proc/locks");
        throw EVerifyProcLocksEntry();
    }

    throw EVerifyProcLocksEntry();
}
// -----------------------------------------------------------------------------

TEST_F(ClusterLockTest, HoldLockFor2Seconds)
{
    FString name;
    name = "HoldLockFor2Seconds";

    FString theFile(FStringFC(),
                    "%s/%s",
                    ClusterLock::LOCK_PATH,
                    name.c_str());

    hlog(HLOG_INFO, "ATTEMPTING TO LOCK %s", name.c_str());
    ClusterLock theLock(name, 5);
    hlog(HLOG_INFO, "GOT LOCK %s", name.c_str()); 
    verifyFileHasEntryInProcLocks(theFile);
    sleep(2);
    verifyFileHasEntryInProcLocks(theFile);

    hlog(HLOG_INFO, "RELEASING LOCK %s", name.c_str());
}
// -----------------------------------------------------------------------------

TEST_F(ClusterLockTest, AnInProcessLockTimeoutDoesNotReleaseYourLock)
{
    FString name;
    name = "AnInProcessLockTimeoutDoesNotReleaseYourLock";

    FString theFile(FStringFC(),
                    "%s/%s",
                    ClusterLock::LOCK_PATH,
                    name.c_str());

    hlog(HLOG_INFO, "ATTEMPTING TO LOCK %s", name.c_str());
    ClusterLock theLock(name, 5);
    hlog(HLOG_INFO, "GOT LOCK %s", name.c_str()); 
    verifyFileHasEntryInProcLocks(theFile);

    ASSERT_THROW(ClusterLock theSameLock(name, 1),
                 EClusterLockTimeout);

    verifyFileHasEntryInProcLocks(theFile);
    hlog(HLOG_INFO, "RELEASING LOCK %s", name.c_str());
}
// -----------------------------------------------------------------------------

TEST_F(ClusterLockTest, StaticMutexesAreFreed)
{
    FString name1("StaticMutexesAreFreed1");
    FString name2("StaticMutexesAreFreed2");
    ASSERT_EQ(0u, ClusterLock::NumLocks());
    {
        ClusterLock lock1(name1, 1);
        ASSERT_EQ(1u, ClusterLock::NumLocks());
        {
            ClusterLock lock2(name2, 1);
            ASSERT_EQ(2u, ClusterLock::NumLocks());
        }
        ASSERT_EQ(1u, ClusterLock::NumLocks());
    }
    ASSERT_EQ(0u, ClusterLock::NumLocks());
}
// -----------------------------------------------------------------------------

EXCEPTION_CLASS(ELockThreadTimeout);
/**
 * LockThread is a thread which performs basic locking/unlocking
 * actions at the will of a control thread.  The Lock/Unlock methods
 * allow the control thread to request specific actions.
 *
 * Assumptions: The thread which calls the LockThread constructor will
 * also call the destructor.  The same thread MUST also be the one
 * which calls Wait().
 */
class LockThread : public Thread
{
public:
    LockThread(const char *lockname)
        : mLockName(lockname),
          mHasLock(false),
          mCompletionCond(mCompletionLock),
          mHoldLock(mCompletionLock) {
        initialized();
    };
    virtual ~LockThread() { FTRACE; deleting(); }
    /**
     * Lock() will notify this LockThread to attempt to obtain the
     * named clusterlock.
     */
    void Lock(void) { FTRACE; mQueue.Add(make_shared<Event>("lock")); Notify(); }
    /**
     * Unlock() will notify this LockThread to unlock the named
     * ClusterLock.
     */
    void Unlock(void) { FTRACE; mQueue.Add(make_shared<Event>("unlock")); Notify(); }
    /**
     * Wait() will not return until the last operation (Lock/Unlock)
     * completes on this LockThread.  Wait() must be called for each
     * operation (it operates completely synchronously).  The
     * operation itself (Lock or Unlock) may complete prior to the
     * Wait() call, but the LockThread will not continue processing
     * the next operation until Wait() has been called.
     */
    void Wait(void) {
        FTRACE;
        if (mCompletionCond.TimedWait(10))
            throw ELockThreadTimeout();
    }

    /**
     * HasLock() will return whether this LockThread currently holds
     * the named clusterlock.  HasLock() may be called at any time
     * (including between a Lock/Unlock and its associated Wait()
     * call).
     */
    bool HasLock(void) { return mHasLock; }
protected:
    virtual void * run(void) {
        while (!mThreadShutdown)
        {
            shared_ptr<Event> e;
            while (e = mQueue.Get())
            {
                if (e->mName == "lock")
                {
                    hlog(HLOG_DEBUG, "locking '%s'", mLockName.c_str());
                    mLock.Lock(mLockName, 10);
                    mHasLock = true;
                }
                else if (e->mName == "unlock")
                {
                    hlog(HLOG_DEBUG, "unlocking '%s'", mLockName.c_str());
                    mLock.Unlock();
                    mHasLock = false;
                }
                AutoUnlockMutex lock(mCompletionLock);
                mCompletionCond.Signal();
            }
            interruptibleSleep(Timespec::FromSeconds(1));
        }
        return NULL;
    }
    FString mLockName;
    ClusterLock mLock;
    bool mHasLock;
    EventQueue mQueue;

    /**
     * mCompletionLock guards the mCompletionCond, and is locked as
     * soon as the thread starts up.
     */
    Mutex mCompletionLock;
    /**
     * mCompletionCond is Signal()led at the completion of each
     * Lock/Unlock operation to allow an external control thread to
     * synchronize on our state.
     */
    ThreadCondition mCompletionCond;
    AutoUnlockMutex mHoldLock;
};

TEST_F(ClusterLockTest, MultiThreaded)
{
    FString name("MultiThreaded");
    LockThread t1(name), t2(name);
    ASSERT_EQ(false, t1.HasLock());
    ASSERT_EQ(false, t2.HasLock());
    ASSERT_EQ(0u, ClusterLock::NumLocks());
    t1.Lock();
    ASSERT_NO_THROW(t1.Wait());
    t2.Lock();
    // there is currently no way to guarantee a wait until we know t2
    // is blocked on the Lock().  Let's wait a bit:
    usleep(100000);
    ASSERT_EQ(true, t1.HasLock());
    ASSERT_EQ(false, t2.HasLock());
    ASSERT_EQ(1u, ClusterLock::NumLocks());
    t1.Unlock();
    ASSERT_NO_THROW(t1.Wait());
    ASSERT_NO_THROW(t2.Wait());
    ASSERT_EQ(false, t1.HasLock());
    ASSERT_EQ(true, t2.HasLock());
    ASSERT_EQ(1u, ClusterLock::NumLocks());
    t1.Lock();
    usleep(100000);
    ASSERT_EQ(false, t1.HasLock());
    ASSERT_EQ(true, t2.HasLock());
    ASSERT_EQ(1u, ClusterLock::NumLocks());
    t2.Unlock();
    ASSERT_NO_THROW(t2.Wait());
    ASSERT_NO_THROW(t1.Wait());
    ASSERT_EQ(true, t1.HasLock());
    ASSERT_EQ(false, t2.HasLock());
    ASSERT_EQ(1u, ClusterLock::NumLocks());
    t1.Unlock();
    ASSERT_NO_THROW(t1.Wait());
    ASSERT_EQ(false, t1.HasLock());
    ASSERT_EQ(false, t2.HasLock());
    ASSERT_EQ(0u, ClusterLock::NumLocks());
};
