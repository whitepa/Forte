#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "FTrace.h"
#include "LogManager.h"

#include "RingBufferCalculator.h"

using namespace std;
using namespace boost;
using namespace Forte;

using ::testing::_;
using ::testing::Return;
using ::testing::UnitTest;

LogManager logManager;

class RingBufferCalculatorUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.BeginLogging(__FILE__ ".log", HLOG_ALL);
        logManager.BeginLogging("//stderr",
                                logManager.GetSingleLevelFromString("UPTO_DEBUG"),
                                HLOG_FORMAT_SIMPLE | HLOG_FORMAT_THREAD);
    }

    static void TearDownTestCase() {
        logManager.EndLogging();
    }

    void SetUp() {
        hlogstream(
            HLOG_INFO, "Starting test "
            << UnitTest::GetInstance()->current_test_info()->name());
        mBuffer.reset(new char[11]);
        for (char i=0; i<11; ++i)
        {
            mBuffer[i] = i;
        }
    }

    void TearDown() {
        hlogstream(
            HLOG_INFO, "ending test "
            << UnitTest::GetInstance()->current_test_info()->name());
    }
    boost::scoped_array<char> mBuffer;
};

TEST_F(RingBufferCalculatorUnitTest, ConstructDelete)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
}

// ring buffer provides the information below for the various scenarios
// . = open space
// HH = PDUHeader (fixed
// p = payload (dynamic)
// o = optional data (dynamic)
//
// read cursor = 0, read size =  0, write cursor = 0, write size = 11, empty
// ...........
TEST_F(RingBufferCalculatorUnitTest, HandlesEmptyBuffer)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    ASSERT_TRUE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(11, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get(), rbc.GetWriteLocation());

    ASSERT_EQ(0, rbc.GetReadLength());
    ASSERT_EQ(0, rbc.GetReadLengthNoWrap());
    ASSERT_THROW(rbc.GetReadLocation(), ERingBufferCalculator);
}

// read cursor = 0, read size = 11, write cursor = 0, write size = 0, !empty
// HHpoHHpHHpo
//
TEST_F(RingBufferCalculatorUnitTest, HandlesFullBuffer)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(11);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_TRUE(rbc.Full());

    ASSERT_EQ(11, rbc.GetReadLength());
    ASSERT_EQ(11, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get(), rbc.GetReadLocation());

    ASSERT_EQ(0, rbc.GetWriteLength());
    ASSERT_THROW(rbc.GetWriteLocation(), ERingBufferCalculator);

    ASSERT_FALSE(rbc.ObjectWillWrap(5));
    ASSERT_FALSE(rbc.ObjectWillWrap(11));
    char buf[5] = { 0, 1, 2, 3, 4 };
    ASSERT_EQ(0, memcmp(buf, rbc.GetReadLocation(), 5));
}

// read cursor = 0, read size =  4, write cursor = 4, write size = 7, !empty
// HHpo.......
TEST_F(RingBufferCalculatorUnitTest, HandlesBufferWithWritesFromFront)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(4);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(4, rbc.GetReadLength());
    ASSERT_EQ(4, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get(), rbc.GetReadLocation());

    ASSERT_EQ(7, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get() + 4, rbc.GetWriteLocation());

    ASSERT_FALSE(rbc.ObjectWillWrap(4));
}

//
// read cursor = 0, read size =  7, write cursor = 7, write size = 4, !empty
// HHpoHHp....
TEST_F(RingBufferCalculatorUnitTest, HandlesBufferWithMultipleWritesFromFront)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(4);
    rbc.RecordWrite(3);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(7, rbc.GetReadLength());
    ASSERT_EQ(7, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get(), rbc.GetReadLocation());

    ASSERT_EQ(4, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get() + 7, rbc.GetWriteLocation());

    ASSERT_FALSE(rbc.ObjectWillWrap(4));
    ASSERT_FALSE(rbc.ObjectWillWrap(7));
}

//
// read cursor = 4, read size =  7, write cursor = 0, write size = 4, !empty
// ....HHpHHpo
TEST_F(RingBufferCalculatorUnitTest, HandlesReadFromBeginning)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(4);
    rbc.RecordWrite(3);
    rbc.RecordWrite(4);
    rbc.RecordRead(4);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(7, rbc.GetReadLength());
    ASSERT_EQ(7, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get() + 4, rbc.GetReadLocation());

    ASSERT_EQ(4, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get(), rbc.GetWriteLocation());

    ASSERT_FALSE(rbc.ObjectWillWrap(7));
}

//
// read cursor = 7, read size =  4, write cursor = 0, write size = 7, !empty
// .......HHpo
TEST_F(RingBufferCalculatorUnitTest, HandlesReadFromMiddle)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(4);
    rbc.RecordWrite(3);
    rbc.RecordWrite(4);
    rbc.RecordRead(4);
    rbc.RecordRead(3);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(4, rbc.GetReadLength());
    ASSERT_EQ(4, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get() + 7, rbc.GetReadLocation());

    ASSERT_EQ(7, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get(), rbc.GetWriteLocation());

    ASSERT_FALSE(rbc.ObjectWillWrap(4));
}

//
// read cursor = 0, read size =  0, write cursor = 0, write size = 11, empty
// ...........
TEST_F(RingBufferCalculatorUnitTest, HandlesReadToEnd)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(4);
    rbc.RecordWrite(3);
    rbc.RecordWrite(4);
    rbc.RecordRead(4);
    rbc.RecordRead(3);
    rbc.RecordRead(4);

    ASSERT_TRUE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(0, rbc.GetReadLength());
    ASSERT_EQ(0, rbc.GetReadLengthNoWrap());
    ASSERT_THROW(rbc.GetReadLocation(), ERingBufferCalculator);

    ASSERT_EQ(11, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get(), rbc.GetWriteLocation());
}

//
// read cursor = 3, read size =  3, write cursor = 6, write size = 5, !empty
// ...HHp.....
TEST_F(RingBufferCalculatorUnitTest, HandlesReadWriteInMiddle)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(3);
    rbc.RecordWrite(3);
    rbc.RecordRead(3);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(3, rbc.GetReadLength());
    ASSERT_EQ(3, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get() + 3, rbc.GetReadLocation());

    ASSERT_EQ(5, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get() + 6, rbc.GetWriteLocation());

    char buf[3] = { 3, 4, 5 };
    char copiedBuf[3];
    rbc.ObjectCopy(copiedBuf, 3);
    ASSERT_EQ(0, memcmp(buf, copiedBuf, 3));
}

//
// read cursor = 10, read size = 3, write cursor = 2, write size = 8, !empty
// Hp........H
TEST_F(RingBufferCalculatorUnitTest, ProvidesInfoForWrappedRead)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(11);
    rbc.RecordRead(10);
    rbc.RecordWrite(2);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_FALSE(rbc.Full());

    ASSERT_EQ(3, rbc.GetReadLength());
    ASSERT_EQ(1, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get() + 10, rbc.GetReadLocation());

    ASSERT_EQ(8, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get() + 2, rbc.GetWriteLocation());

    ASSERT_TRUE(rbc.ObjectWillWrap(3));

    char buf[3] = { 10, 0, 1 };
    char copiedBuf[3];
    rbc.ObjectCopy(copiedBuf, 3);
    ASSERT_EQ(0, memcmp(buf, copiedBuf, 3));
}


TEST_F(RingBufferCalculatorUnitTest, RollsOverOnRecordWriteToEnd)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    rbc.RecordWrite(11);
    rbc.RecordRead(1);
    rbc.RecordWrite(1);

    ASSERT_FALSE(rbc.Empty());
    ASSERT_TRUE(rbc.Full());

    ASSERT_EQ(11, rbc.GetReadLength());
    ASSERT_EQ(10, rbc.GetReadLengthNoWrap());
    ASSERT_EQ(mBuffer.get()+1, rbc.GetReadLocation());

    ASSERT_EQ(0, rbc.GetWriteLength());
    rbc.RecordRead(1);
    ASSERT_EQ(1, rbc.GetWriteLength());
    ASSERT_EQ(mBuffer.get()+1, rbc.GetWriteLocation());

    //x.xxxxxxxxxx
    ASSERT_FALSE(rbc.ObjectWillWrap(5));
    ASSERT_EQ(10, rbc.GetReadLength());
    ASSERT_TRUE(rbc.ObjectWillWrap(10));
    char buf[5] = { 2, 3, 4, 5, 6 };
    ASSERT_EQ(0, memcmp(buf, rbc.GetReadLocation(), 5));
}

TEST_F(RingBufferCalculatorUnitTest, CopyObjectWorksForWrappingObjects)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    ASSERT_EQ(11, rbc.GetWriteLength());
    rbc.RecordWrite(11);
    //xxxxxxxxxxx
    {
        char buf[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        char copiedBuf[11];
        ASSERT_FALSE(rbc.ObjectWillWrap(11));
        rbc.ObjectCopy(copiedBuf, 11);
        ASSERT_EQ(0, memcmp(buf, copiedBuf, 11));
    }

    ASSERT_EQ(11, rbc.GetReadLength());
    ASSERT_EQ(11, rbc.GetReadLengthNoWrap());

    rbc.RecordRead(1);
    ASSERT_EQ(1, rbc.GetWriteLength());
    rbc.RecordWrite(1);
    //xxxxxxxxxxx
    // |
    {
        char buf[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0 };
        char copiedBuf[11];
        ASSERT_TRUE(rbc.ObjectWillWrap(11));
        rbc.ObjectCopy(copiedBuf, 11);
        ASSERT_EQ(0, memcmp(buf, copiedBuf, 11));
    }

    ASSERT_EQ(11, rbc.GetReadLength());
    ASSERT_EQ(10, rbc.GetReadLengthNoWrap());

    rbc.RecordRead(2);
    ASSERT_EQ(2, rbc.GetWriteLength());
    rbc.RecordWrite(2);
    //xxxxxxxxxxx
    //   |
    {
        char buf[] = { 3, 4, 5, 6, 7, 8, 9, 10, 0, 1, 2 };
        char copiedBuf[11];
        ASSERT_TRUE(rbc.ObjectWillWrap(11));
        rbc.ObjectCopy(copiedBuf, 11);
        ASSERT_EQ(0, memcmp(buf, copiedBuf, 11));
    }

    ASSERT_EQ(11, rbc.GetReadLength());
    ASSERT_EQ(8, rbc.GetReadLengthNoWrap());

    rbc.RecordRead(2);
    ASSERT_EQ(2, rbc.GetWriteLength());
    //xxx..xxxxxx
    {
        char buf[] = { 5, 6, 7, 8, 9, 10, 0, 1, 2 };
        char copiedBuf[9];
        ASSERT_TRUE(rbc.ObjectWillWrap(11));
        rbc.ObjectCopy(copiedBuf, 9);
        ASSERT_EQ(0, memcmp(buf, copiedBuf, 9));
    }

    ASSERT_EQ(9, rbc.GetReadLength());
    ASSERT_EQ(6, rbc.GetReadLengthNoWrap());

    rbc.RecordRead(5);
    ASSERT_EQ(7, rbc.GetWriteLength());
    //xxx.......x
    {
        char buf[] = { 10, 0, 1, 2 };
        char copiedBuf[4];
        ASSERT_TRUE(rbc.ObjectWillWrap(4));
        rbc.ObjectCopy(copiedBuf, 4);
        ASSERT_EQ(0, memcmp(buf, copiedBuf, 4));
    }

    ASSERT_EQ(4, rbc.GetReadLength());
    ASSERT_EQ(1, rbc.GetReadLengthNoWrap());

    rbc.RecordRead(1);
    ASSERT_EQ(8, rbc.GetWriteLength());
    //xxx........
    {
        char buf[] = { 0, 1, 2 };
        char copiedBuf[3];
        ASSERT_FALSE(rbc.ObjectWillWrap(3));
        rbc.ObjectCopy(copiedBuf, 3);
        ASSERT_EQ(0, memcmp(buf, copiedBuf, 3));
    }

    ASSERT_EQ(3, rbc.GetReadLength());
    ASSERT_EQ(3, rbc.GetReadLengthNoWrap());

    rbc.RecordRead(1);
    ASSERT_EQ(8, rbc.GetWriteLength());
    rbc.RecordWrite(8);
    //.xxxxxxxxxx
    {
        char buf[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        char copiedBuf[10];
        ASSERT_FALSE(rbc.ObjectWillWrap(10));
        rbc.ObjectCopy(copiedBuf, 10);
        ASSERT_EQ(0, memcmp(buf, copiedBuf, 10));
    }

    ASSERT_EQ(10, rbc.GetReadLength());
    ASSERT_EQ(10, rbc.GetReadLengthNoWrap());

}

TEST_F(RingBufferCalculatorUnitTest, RecordReadWorksForWrappingObjects)
{
    RingBufferCalculator rbc(mBuffer.get(), 11);
    ASSERT_EQ(11, rbc.GetWriteLength());
    rbc.RecordWrite(11);

    rbc.RecordRead(5);
    rbc.RecordWrite(5);
    //01234567890
    //     |
    ASSERT_EQ(0, rbc.GetWriteLength());
    ASSERT_EQ(11, rbc.GetReadLength());
    ASSERT_EQ(6, rbc.GetReadLengthNoWrap());
    ASSERT_TRUE(rbc.ObjectWillWrap(11));
    ASSERT_TRUE(rbc.Full());
    char buf[] = { 5, 6, 7, 8, 9, 10, 0, 1, 2, 3, 4 };
    char copiedBuf[11];

    rbc.ObjectCopy(copiedBuf, 11);
    rbc.RecordRead(11);
    ASSERT_EQ(0, memcmp(buf, copiedBuf, 11));
    ASSERT_EQ(6, rbc.GetWriteLength());
    ASSERT_EQ(0, rbc.GetReadLength());
    ASSERT_EQ(0, rbc.GetReadLengthNoWrap());
    ASSERT_TRUE(rbc.Empty());
}
