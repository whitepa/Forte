#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "FTrace.h"
#include "LogManager.h"
#include "FunctionThread.h"

#include <boost/bind.hpp>

#include "RWLock.h"

using namespace std;
using namespace boost;
using namespace Forte;

LogManager logManager;

class RWLockUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
};

TEST_F(RWLockUnitTest, ConstructDelete)
{
    FTRACE;
    RWLock rwl;

    {
        AutoReadUnlock lock(rwl);
    }

    {
        AutoWriteUnlock lock(rwl);
    }
}

TEST_F(RWLockUnitTest, MultipleReadLocks)
{
    FTRACE;
    RWLock rwl;

    {
        AutoReadUnlock lock(rwl);
        AutoReadUnlock lock2(rwl);
        AutoReadUnlock lock3(rwl);
    }
}

struct ReaderObj
{
public:
    ReaderObj() : mAssertIfRead(false) {}

    void Read() {
        if (mAssertIfRead) {
            abort();
        }
    }

    void SetAssertIfRead(bool assertIfRead) {
        if (mAssertIfRead == assertIfRead)
        {
            abort();
        }
        mAssertIfRead = assertIfRead;
    }

private:
    bool mAssertIfRead;
};

void GetReadLockThread(RWLock& rwl, ReaderObj& obj)
{
    for (int i=0; i<10000; ++i)
    {
        AutoReadUnlock lock(rwl);
        obj.Read();
    }
    hlog(HLOG_INFO, "Read thread complete");
}

void GetWriteLockThread(RWLock& rwl, ReaderObj& obj)
{
    for (int i=0; i<100; ++i)
    {
        usleep(1000);
        AutoWriteUnlock lock(rwl);
        obj.SetAssertIfRead(true);
        usleep(10000);
        obj.SetAssertIfRead(false);
    }
    hlog(HLOG_INFO, "Write thread complete");
}

TEST_F(RWLockUnitTest, CanQueueWriteLock_TriggerReaders_ProtectedData)
{
    FTRACE;
    RWLock rwl;
    ReaderObj obj;

    std::vector<boost::shared_ptr<FunctionThread> > threadGroup;

    // readers
    for (int i=0; i<10; ++i)
    {
        threadGroup.push_back(
            boost::shared_ptr<FunctionThread>(
                new FunctionThread(
                    boost::bind(&GetReadLockThread,
                                boost::ref(rwl),
                                boost::ref(obj)),
                    "rdr")));
    }


    // writers
    for (int i=0; i<10; ++i)
    {
        threadGroup.push_back(
            boost::shared_ptr<FunctionThread>(
                new FunctionThread(
                    boost::bind(&GetWriteLockThread,
                                boost::ref(rwl),
                                boost::ref(obj)),
                    "writer"
                    )));
    }

    std::for_each(
        threadGroup.begin(),
        threadGroup.end(),
        boost::bind(&FunctionThread::Begin, _1));

    std::for_each(
        threadGroup.begin(),
        threadGroup.end(),
        boost::bind(&FunctionThread::WaitForShutdown, _1));
}
