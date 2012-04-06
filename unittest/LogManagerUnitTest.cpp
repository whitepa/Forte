#include <gtest/gtest.h>
#include "LogManager.h"
#include "FTrace.h"
#include <boost/make_shared.hpp>

using namespace std;
using namespace boost;
using namespace Forte;


LogManager logManager;

void outputAllMessagesFunction(int howDeep=1)
{
    FTRACE;
    hlog(HLOG_SQL, "HLOG_SQL");
    hlog(HLOG_TRACE, "HLOG_TRACE");
    hlog(HLOG_DEBUG4, "HLOG_DEBUG4");
    hlog(HLOG_DEBUG3, "HLOG_DEBUG3");
    hlog(HLOG_DEBUG2, "HLOG_DEBUG2");
    hlog(HLOG_DEBUG1, "HLOG_DEBUG1");
    hlog(HLOG_DEBUG, "HLOG_DEBUG");
    hlog(HLOG_INFO, "HLOG_INFO");
    hlog(HLOG_NOTICE, "HLOG_NOTICE");
    hlog(HLOG_WARN, "HLOG_WARN");
    hlog(HLOG_ERR, "HLOG_ERR");
    hlog(HLOG_CRIT, "HLOG_CRIT");
    hlog(HLOG_ALERT, "HLOG_ALERT");
    hlog(HLOG_EMERG, "HLOG_EMERG");

    if (howDeep > 0)
    {
        outputAllMessagesFunction(howDeep-1);
    }
}

TEST(LogManagerUnitTest, WatchOutput)
{
    logManager.BeginLogging("//stdout",
                            HLOG_NODEBUG,
                            HLOG_FORMAT_TIMESTAMP | HLOG_FORMAT_MESSAGE);

    outputAllMessagesFunction();

    logManager.EndLogging("//stdout");
}

TEST(LogManagerUnitTest, WatchOutputWithDepth)
{
    logManager.BeginLogging(
        "//stdout", HLOG_NODEBUG,
        HLOG_FORMAT_TIMESTAMP | HLOG_FORMAT_MESSAGE | HLOG_FORMAT_DEPTH);

    outputAllMessagesFunction(3);

    logManager.EndLogging("//stdout");
}

TEST(LogManagerUnitTest, TestOutput)
{
    FString filename = "./testoutputlog";
    logManager.BeginLogging(filename,
                            HLOG_NODEBUG,
                            HLOG_FORMAT_TIMESTAMP | HLOG_FORMAT_MESSAGE);

    outputAllMessagesFunction();

    EXPECT_EQ(0, system("/bin/grep CRIT ./testoutputlog"));
    EXPECT_NE(0, system("/bin/grep DEBUG ./testoutputlog"));

    logManager.EndLogging(filename);
    unlink(filename.c_str());
}

class LogClient : public Forte::Thread
{
public:
    LogClient(unsigned long& stats):mStats(stats), mRun(false), mFinish(false), mCondition(mMutexStart), mConditionFinish(mMutexFinish)
    {
        initialized();
    }

    void *run()
    {
        WaitForStart();
        hlogstream(HLOG_ERR, "delayed log write initiated");

        {
            AutoUnlockMutex lock(mMutexStats);
            ++mStats;
        }

        NotifyFinish();
        return 0;
    }

    void Start()
    {
        AutoUnlockMutex lock(mMutexStart);
        mRun = true;
        mCondition.Signal();
    }

    void NotifyFinish()
    {
        AutoUnlockMutex lock(mMutexFinish);
        mFinish = true;
        mConditionFinish.Signal();
    }

    void WaitForStart()
    {
        AutoUnlockMutex lock(mMutexStart);
        while(! mRun)
            mCondition.Wait();
    }

    void WaitForFinish()
    {
        AutoUnlockMutex lock(mMutexFinish);
        while(! mFinish)
            mConditionFinish.Wait();
    }

    unsigned long& mStats;
    bool mRun;
    bool mFinish;
    Forte::Mutex mMutexStart;
    Forte::Mutex mMutexFinish;
    Forte::Mutex mMutexStats;
    Forte::ThreadCondition mCondition;
    Forte::ThreadCondition mConditionFinish;
};

class CountedWriteLogfile : public Forte::Logfile
{
public:
    typedef Forte::Logfile base_type;

    CountedWriteLogfile(unsigned long& ctr, const unsigned long& maxCtr, const time_t& waitSecs, const time_t& delaySecs)
        : base_type("", 0, HLOG_ERR, false),
        mCtr(ctr), mMaxCtr(maxCtr), mWaitSecs(waitSecs), mDelaySecs(delaySecs), mCondition(mMutex)
    {
    }

    void Write(const Forte::LogMsg& msg)
    {
        try
        {
            Forte::Thread::MyThread()->InterruptibleSleep(Timespec(mDelaySecs, 0));

            AutoUnlockMutex lock(mMutex);

            if(! (++mCtr < mMaxCtr))
            {
                mCondition.Broadcast();
            }
            else
            {
                if(mCtr < mMaxCtr)
                {
                    mCondition.TimedWait(Timespec(mWaitSecs, 0));
                }
            }
        }
        catch(Forte::EThreadUnknown& e)
        {
        }
    }

    unsigned long& mCtr;
    const unsigned long mMaxCtr;
    const time_t mWaitSecs;
    const time_t mDelaySecs;
    Forte::Mutex mMutex;
    Forte::ThreadCondition mCondition;
};

// tests:
//
// 1) all threads can enter the Logfile Write simultaneously
// 2) at least 200 simulated log writes per second (assumming simultaneous writes)
TEST(LogManagerUnitTest, TestMultiThreadedConcurrency)
{
    static const time_t MAX_THREAD_WAIT_SECS(3);
    static const time_t LOG_WRITE_DELAY_SECS(1);
    static const unsigned long MAX_THREADS(600);
    unsigned long ctr(0);

    logManager.EndLogging();
    logManager.BeginLogging(shared_ptr<Logfile>(
                new CountedWriteLogfile(ctr, MAX_THREADS, MAX_THREAD_WAIT_SECS, LOG_WRITE_DELAY_SECS)));

    vector<boost::shared_ptr<LogClient> > clients;

    unsigned long stats(0);

    for(unsigned long i=0; i<MAX_THREADS; i++)
    {
        boost::shared_ptr<LogClient> client(new LogClient(stats));
        clients.push_back(client);
        client->WaitForInitialize();
    }

    for(unsigned long i=0; i<MAX_THREADS; i++)
    {
        clients[i]->Start();
    }

    for(unsigned long i=0; i<MAX_THREADS; i++)
    {
        clients[i]->WaitForFinish();
    }

    for(unsigned long i=0; i<MAX_THREADS; i++)
    {
        clients[i]->WaitForShutdown();
    }

    logManager.EndLogging();

    EXPECT_EQ(MAX_THREADS, ctr);
}
