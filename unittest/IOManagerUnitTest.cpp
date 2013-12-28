#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <libaio.h>
#include "LogManager.h"
#include "FTrace.h"
#include "FileSystemImpl.h"
#include "IOManager.h"

using namespace Forte;

LogManager logManager;

class IOManagerUnitTest : public ::testing::Test
{
public:
    IOManagerUnitTest() :
        mCond(mLock),mNumCompletions(0)
    {
        hlog(HLOG_INFO, "initializing test case");
        mTmpfile = mFileSystem.MakeTemporaryFile("/tmp/IOManagerUnitTest-XXXXXX");
//        mTmpfile = "tmpfile";
    }
    ~IOManagerUnitTest() {
        mFileSystem.Unlink(mTmpfile);
    }

    static void SetUpTestCase() {
        logManager.BeginLogging("//stderr");
    }
    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    static void Notify(const IORequest &req) {
        FTRACE;
        hlog(HLOG_INFO, "IO request %lu completed, result is %ld",
             req.GetRequestNumber(), req.GetResult());
        IOManagerUnitTest &ut(*(reinterpret_cast<IOManagerUnitTest*>(req.GetUserData())));
        AutoUnlockMutex lock(ut.mLock);
        ++ut.mNumCompletions;
//        hlog(HLOG_DEBUG, "mNumCompletions now %d", ut.mNumCompletions);
        ut.mCond.Signal();
    }
    void Wait(const int numCompletions = 1) {
        AutoUnlockMutex lock(mLock);
        while (mNumCompletions < numCompletions)
        {
            // hlog(HLOG_DEBUG, "mNumCompletions=%d numCompletions=%d",
            //      mNumCompletions, numCompletions);
            mCond.Wait();
        }
    }
    Mutex mLock;
    ThreadCondition mCond;
    int mNumCompletions;
    FileSystemImpl mFileSystem;
    FString mTmpfile;
};

TEST_F(IOManagerUnitTest, DirectIOFileOpen)
{
    FTRACE;
    FileSystemImpl fs;

    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);
}

TEST_F(IOManagerUnitTest, DirectIOFileReadAligned)
{
    FTRACE;
    FileSystemImpl fs;

    // populate the file with some data
    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    // Allocate a buffer
    char *buf = 0;
    size_t len = 4096;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/len));
    memset(buf, 0, len);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    boost::shared_ptr<IORequest> req = iomgr->NewRequest();
    req->SetOp(IORequest::READ);
    req->SetCallback(IOManagerUnitTest::Notify);
    req->SetUserData(this);
    req->SetBuffer(buf, len);
    req->SetOffset(4096);
    req->SetFD(fd);
    uint64_t reqnum = req->GetRequestNumber();
    req->Begin();
    hlog(HLOG_INFO, "submitted IO request %lu", reqnum);
    Wait();
    hlog(HLOG_INFO, "got result %ld", req->GetResult());
    ASSERT_EQ(len, req->GetResult());
    free(buf);
}
TEST_F(IOManagerUnitTest, DirectIOFileReadUnaligned)
{
    FTRACE;
    FileSystemImpl fs;

    // populate the file with some data
    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    // Allocate a buffer
    char *buf = 0;
    size_t len = 1024;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/4096));
    memset(buf, 0, len);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    boost::shared_ptr<IORequest> req = iomgr->NewRequest();
    req->SetOp(IORequest::READ);
    req->SetCallback(IOManagerUnitTest::Notify);
    req->SetUserData(this);
    req->SetBuffer(buf+128, 512); // misaligned
    req->SetOffset(4096);
    req->SetFD(fd);
    uint64_t reqnum = req->GetRequestNumber();
    req->Begin();
    hlog(HLOG_INFO, "submitted IO request %lu", reqnum);
    Wait();
    hlog(HLOG_INFO, "got result %ld", req->GetResult());
    if (req->GetResult() < 0)
        hlog(HLOG_INFO, "error is %s", strerror(-req->GetResult()));
    ASSERT_EQ(-22, req->GetResult());
    free(buf);
}

TEST_F(IOManagerUnitTest, DirectIOFileWriteAligned)
{
    FTRACE;
    FileSystemImpl fs;

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_RDWR | O_CREAT | O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    // Allocate a buffer
    char *buf = 0;
    size_t len = 4096;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/len));
    memset(buf, 65, len);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    boost::shared_ptr<IORequest> req = iomgr->NewRequest();
    req->SetOp(IORequest::WRITE);
    req->SetBuffer(buf, len);
    req->SetOffset(0);
    req->SetFD(fd);
    req->SetCallback(IOManagerUnitTest::Notify);
    req->SetUserData(this);
    hlog(HLOG_INFO, "got here");
    uint64_t reqnum = req->GetRequestNumber();
    req->Begin();
    hlog(HLOG_INFO, "submitted IO request %lu", reqnum);
    Wait();
    ASSERT_EQ(4096, req->GetResult());
    free(buf);
}

TEST_F(IOManagerUnitTest, DirectIOFileWriteUnaligned)
{
    FTRACE;
    FileSystemImpl fs;

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_RDWR | O_CREAT | O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    // Allocate a buffer
    char *buf = 0;
    ssize_t len = 1024;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/4096));
    off_t interval = 64;
    int i = 0;
    for (off_t offset = 0; offset < 4096; offset += interval)
        memset(buf + offset, ++i, interval);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    boost::shared_ptr<IORequest> req = iomgr->NewRequest();
    req->SetOp(IORequest::WRITE);
    req->SetBuffer(buf+128, len); // unaligned
    req->SetOffset(0);
    req->SetFD(fd);
    req->SetCallback(IOManagerUnitTest::Notify);
    req->SetUserData(this);
    hlog(HLOG_INFO, "got here");
    uint64_t reqnum = req->GetRequestNumber();
    req->Begin();
    hlog(HLOG_INFO, "submitted IO request %lu", reqnum);
    Wait();
    hlog(HLOG_INFO, "got result %ld", req->GetResult());
    if (req->GetResult() < 0)
        hlog(HLOG_INFO, "error is %s", strerror(-req->GetResult()));
    ASSERT_EQ(-22, req->GetResult());
    free(buf);
}

TEST_F(IOManagerUnitTest, MultipleReads)
{
    FTRACE;
    FileSystemImpl fs;

    // populate the file with some data
    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    // Read a series of 512 byte blocks
    ssize_t total = 512*512;
    ssize_t each = 512;
    char *buf = 0;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/total));
    memset(buf, 1, total);
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        if (*a != 0x01010101)
            hlog(HLOG_ERR, "a != 0:  i=%u buf=%p addr=%p",
                 i, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ(0x01010101, *a);
    }
    std::map<uint64_t, boost::shared_ptr<IORequest> > requestMap;
    for (unsigned int i = 0; i < total / each; ++i)
    {
        // submit IO request
        boost::shared_ptr<IORequest> req = iomgr->NewRequest();
        req->SetOp(IORequest::READ);
        req->SetCallback(IOManagerUnitTest::Notify);
        req->SetUserData(this);
        req->SetBuffer(buf+(each*i), each);
        req->SetOffset(each*i);
        req->SetFD(fd);
        uint64_t reqnum = req->GetRequestNumber();
        ASSERT_NO_THROW(req->Begin());
        requestMap[reqnum] = req;
        EXPECT_EQ(requestMap.size(), i+1); // (check for unique reqnums)
    }
    Wait(total/each);
    foreach (const IORequestPair &p, requestMap)
    {
        const IORequestPtr &req(p.second);
        // hlog(HLOG_INFO, "req %lu got result %ld %s", req->GetRequestNumber(),
        //      req->GetResult(), req->GetResult() > 0 ? "" : strerror(-req->GetResult()));
        EXPECT_EQ(each, req->GetResult());
    }
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        if (*a != 0)
            hlog(HLOG_ERR, "a != 0:  i=%u buf=%p addr=%p",
                 i, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ(0, *a);
    }
    ASSERT_EQ(total/each, mNumCompletions);
    free(buf);
}

// Multiple Writes
TEST_F(IOManagerUnitTest, MultipleWrites)
{
    FTRACE;
    FileSystemImpl fs;

    // populate the file with some data
    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_RDWR | O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    // Read a series of 512 byte blocks
    ssize_t total = 512*512;
    ssize_t each = 512;
    char *buf = 0;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/total));
    memset(buf, 1, total);
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        if (*a != 0x01010101)
            hlog(HLOG_ERR, "a != 0:  i=%u buf=%p addr=%p",
                 i, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ(0x01010101, *a);
    }
    std::map<uint64_t, boost::shared_ptr<IORequest> > requestMap;
    for (int i = 0; i < total / each; ++i)
    {
        // submit IO request
        boost::shared_ptr<IORequest> req = iomgr->NewRequest();
        req->SetOp(IORequest::WRITE);
        req->SetCallback(IOManagerUnitTest::Notify);
        req->SetUserData(this);
        req->SetBuffer(buf+(each*i), each);
        req->SetOffset(each*i);
        req->SetFD(fd);
        uint64_t reqnum = req->GetRequestNumber();
        ASSERT_NO_THROW(req->Begin(););
        requestMap[reqnum] = req;
        EXPECT_EQ(requestMap.size(), i+1); // (check for unique reqnums)
    }
    Wait(total/each);
    foreach (const IORequestPair &p, requestMap)
    {
        const IORequestPtr &req(p.second);
        // hlog(HLOG_INFO, "req %lu got result %ld %s", req->GetRequestNumber(),
        //      req->GetResult(), req->GetResult() > 0 ? "" : strerror(-req->GetResult()));
        EXPECT_EQ(each, req->GetResult());
    }
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        if (*a != 0x01010101)
            hlog(HLOG_ERR, "a != 0x01010101:  i=%u buf=%p addr=%p",
                 i, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ(0x01010101, *a);
    }
    ASSERT_EQ(total/each, mNumCompletions);
    free(buf);
}

// Read/Write Mix
TEST_F(IOManagerUnitTest, ReadWriteMix)
{
    FTRACE;
    FileSystemImpl fs;

    // populate the file with some data
    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_RDWR | O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    ssize_t total = 512*512;
    ssize_t each = 512;
    char *buf = 0;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/total));
    memset(buf, 1, total);
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        if (*a != 0x01010101)
            hlog(HLOG_ERR, "a != 0:  i=%u buf=%p addr=%p",
                 i, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ(0x01010101, *a);
    }
    std::map<uint64_t, boost::shared_ptr<IORequest> > requestMap;
    for (int i = 0; i < total / each; ++i)
    {
        // submit IO request
        boost::shared_ptr<IORequest> req = iomgr->NewRequest();
        if (i % 2 == 0)
            req->SetOp(IORequest::READ);
        else
            req->SetOp(IORequest::WRITE);
        req->SetCallback(IOManagerUnitTest::Notify);
        req->SetUserData(this);
        req->SetBuffer(buf+(each*i), each);
        req->SetOffset(each*i);
        req->SetFD(fd);
        uint64_t reqnum = req->GetRequestNumber();
        ASSERT_NO_THROW(req->Begin(););
        requestMap[reqnum] = req;
        EXPECT_EQ(requestMap.size(), i+1); // (check for unique reqnums)
    }
    Wait(total/each);
    foreach (const IORequestPair &p, requestMap)
    {
        const IORequestPtr &req(p.second);
        // hlog(HLOG_INFO, "req %lu got result %ld %s", req->GetRequestNumber(),
        //      req->GetResult(), req->GetResult() > 0 ? "" : strerror(-req->GetResult()));
        EXPECT_EQ(each, req->GetResult());
    }
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        int b = i / ((total/each) / sizeof(uint32_t));
        if ((b % 2 == 0 && *a != 0) ||
            (b % 2 != 0 && *a != 0x01010101))
            hlog(HLOG_ERR, "a has unexpected value of 0x%x:  i=%u b=%d buf=%p addr=%p",
                 *a, i, b, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ((b % 2 == 0) ? 0 : 0x01010101, *a);
    }
    ASSERT_EQ(total/each, mNumCompletions);
    free(buf);
}

TEST_F(IOManagerUnitTest, BlockFutureRequests)
{
    FileSystemImpl fs;

    // populate the file with some data
    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    // Allocate a buffer
    char *buf = 0;
    size_t len = 4096;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/len));
    memset(buf, 0, len);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    boost::shared_ptr<IORequest> req = iomgr->NewRequest();
    req->SetOp(IORequest::READ);
    req->SetCallback(IOManagerUnitTest::Notify);
    req->SetUserData(this);
    req->SetBuffer(buf, len);
    req->SetOffset(4096);
    req->SetFD(fd);
    uint64_t reqnum = req->GetRequestNumber();

    // block requests
    iomgr->BlockFutureRequests();
    ASSERT_THROW(req->Begin(), ERequestBlocked);

    iomgr->UnblockFutureRequests();
    ASSERT_NO_THROW(req->Begin(););

    hlog(HLOG_INFO, "submitted IO request %lu", reqnum);
    Wait();
    hlog(HLOG_INFO, "got result %ld", req->GetResult());
    ASSERT_EQ(len, req->GetResult());
    free(buf);
}

TEST_F(IOManagerUnitTest, WaitForAllRequestsToComplete)
{
    FTRACE;
    FileSystemImpl fs;

    // populate the file with some data
    system(FString("/bin/dd bs=1024 count=10000 if=/dev/zero of=" + mTmpfile));

    // Open the file with direct IO
    AutoFD fd(open(mTmpfile, O_RDWR | O_DIRECT));
    ASSERT_NE(fd.GetFD(), -1);

    boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(32);

    ssize_t total = 512*512;
    ssize_t each = 512;
    char *buf = 0;
    ASSERT_EQ(0, posix_memalign(reinterpret_cast<void **>(&buf), /*align*/4096, /*size*/total));
    memset(buf, 1, total);
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        if (*a != 0x01010101)
            hlog(HLOG_ERR, "a != 0:  i=%u buf=%p addr=%p",
                 i, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ(0x01010101, *a);
    }
    std::map<uint64_t, boost::shared_ptr<IORequest> > requestMap;
    for (int i = 0; i < total / each; ++i)
    {
        // submit IO request
        boost::shared_ptr<IORequest> req = iomgr->NewRequest();
        if (i % 2 == 0)
            req->SetOp(IORequest::READ);
        else
            req->SetOp(IORequest::WRITE);
        req->SetCallback(IOManagerUnitTest::Notify);
        req->SetUserData(this);
        req->SetBuffer(buf+(each*i), each);
        req->SetOffset(each*i);
        req->SetFD(fd);
        uint64_t reqnum = req->GetRequestNumber();
        ASSERT_NO_THROW(req->Begin(););
        requestMap[reqnum] = req;
        EXPECT_EQ(requestMap.size(), i+1); // (check for unique reqnums)
    }

    iomgr->WaitForAllPendingRequestsToComplete();
    ASSERT_EQ(0, iomgr->GetNumberOfPendingRequests());
    Wait(total/each);
    for(unsigned int i = 0; i < (total/sizeof(uint32_t)); ++i)
    {
        uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
        int b = i / ((total/each) / sizeof(uint32_t));
        if ((b % 2 == 0 && *a != 0) ||
            (b % 2 != 0 && *a != 0x01010101))
            hlog(HLOG_ERR, "a has unexpected value of 0x%x:  i=%u b=%d buf=%p addr=%p",
                 *a, i, b, buf, reinterpret_cast<uint32_t *>(buf) + i);
        ASSERT_EQ((b % 2 == 0) ? 0 : 0x01010101, *a);
    }
    ASSERT_EQ(total/each, mNumCompletions);
    free(buf);
}

// @TODO the CancelRequest test will need to be an onbox test which
// uses a real hardware device.  This test will not work agains a
// file, since the native linux AIO functionality serializes all
// access to a file, and the request is already complete at the
// completion of the io_submit call.

// TEST_F(IOManagerUnitTest, CancelRequest)
// {
//     FTRACE;
//     // populate the file with some data
//     system(FString("/bin/dd bs=1024 count=1 seek=500000000 if=/dev/zero of=" + mTmpfile));

//     // Open the file with direct IO
//     AutoFD fd(open(mTmpfile, O_RDWR | O_DIRECT));
//     ASSERT_NE(fd.GetFD(), -1);

//     boost::shared_ptr<IOManager> iomgr = boost::make_shared<IOManager>(512);

//     ssize_t total = 1024 * 1024 * 256;
//     ssize_t each = 1024 * 1024 * 16;
//     char *buf = 0;
//     ASSERT_EQ(0, posix_memalign((void **)&buf, /*align*/4096, /*size*/total));
//     memset(buf, 1, total);
//     for(int i = 0; i < (ssize_t)(total/sizeof(uint32_t)); ++i)
//     {
//         uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
//         if (*a != 0x01010101)
//             hlog(HLOG_ERR, "a != 0:  i=%d buf=%p addr=%p",
//                  i, buf, reinterpret_cast<uint32_t *>(buf) + i);
//         ASSERT_EQ(0x01010101, *a);
//     }
//     std::map<uint64_t, boost::shared_ptr<IORequest> > requestMap;
//     boost::shared_ptr<IORequest> req;
//     uint64_t reqnum;
//     for (int i = 0; i < total/each; ++i)
//     {
//         // submit IO request
//         req = iomgr->NewRequest();
//         if (i % 2 == 0)
//             req->SetOp(IORequest::READ);
//         else
//             req->SetOp(IORequest::WRITE);
//         req->SetCallback(IOManagerUnitTest::Notify);
//         req->SetUserData(this);
//         req->SetBuffer(buf+(each*i), each);
//         req->SetOffset(each*i);
//         req->SetFD(fd);
//         reqnum = req->GetRequestNumber();
//         hlog(HLOG_INFO, "[BEFORE BEGIN]");
//         ASSERT_NO_THROW(req->Begin(););
//         hlog(HLOG_INFO, "[AFTER BEGIN]");
//         if (i == total/each - 1)
//         {
//             hlog(HLOG_INFO, "************** CANCELING request %lu", reqnum);
//             EXPECT_NO_THROW(req->Cancel());
//         }
//         requestMap[reqnum] = req;
//     }
//     Wait(total/each - 1);
//     usleep(100000);
//     foreach (const IORequestPair &p, requestMap)
//     {
//         const IORequestPtr &req(p.second);
//         // hlog(HLOG_INFO, "req %lu got result %ld %s", req->GetRequestNumber(),
//         //      req->GetResult(), req->GetResult() > 0 ? "" : strerror(-req->GetResult()));
//         if (req->GetRequestNumber() == reqnum)
//         {
//             // should be canceled
//             EXPECT_EQ(-1, req->GetResult());
//         }
//         else
//         {
//             EXPECT_EQ(each, req->GetResult());
//         }
//     }
//     for(int i = 0; i < (ssize_t)(total/sizeof(uint32_t)); ++i)
//     {
//         uint32_t *a = reinterpret_cast<uint32_t *>(buf) + i;
//         int b = i / ((total/each) / sizeof(uint32_t));
//         if ((b % 2 == 0 && *a != 0) ||
//             (b % 2 != 0 && *a != 0x01010101))
//             hlog(HLOG_ERR, "a has unexpected value of 0x%x:  i=%d b=%d buf=%p addr=%p",
//                  *a, i, b, buf, reinterpret_cast<uint32_t *>(buf) + i);
//         ASSERT_EQ((b % 2 == 0) ? 0 : 0x01010101, *a);
//     }
//     ASSERT_EQ(total/each, mNumCompletions);
//     free(buf);
// }

// Test aligned read with an odd buffer size, make sure only the
// desired size of the target buffer is modified.
