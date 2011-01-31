#include "LogManager.h"
#include "FileSystem.h"
#include "ProcRunner.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ClusterLock.h"

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
    ASSERT_EQ(0, ClusterLock::NumLocks());
    {
        ClusterLock lock1(name1, 1);
        ASSERT_EQ(1, ClusterLock::NumLocks());
        {
            ClusterLock lock2(name2, 1);
            ASSERT_EQ(2, ClusterLock::NumLocks());
        }
        ASSERT_EQ(1, ClusterLock::NumLocks());
    }
    ASSERT_EQ(0, ClusterLock::NumLocks());
}
// -----------------------------------------------------------------------------
