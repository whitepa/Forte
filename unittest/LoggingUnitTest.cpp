#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "FString.h"
#include "LogManager.h"
#include "FTrace.h"

using namespace Forte;
using namespace boost;
using ::testing::_;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;

class GMockLogfile : public Logfile
{
public:
    GMockLogfile() :Logfile("", 0){}
    ~GMockLogfile(){}
    MOCK_METHOD1(Write, void (const Forte::LogMsg& msg));
    MOCK_METHOD1(FormatMsg, Forte::FString (const Forte::LogMsg& msg));
    MOCK_METHOD1(CustomFormatMsg, Forte::FString (const Forte::LogMsg& msg));
    MOCK_METHOD0(Reopen, bool());
};

typedef boost::shared_ptr<GMockLogfile> GMockLogfilePtr;


class LoggingTest : public ::testing::Test
{
protected:
    void SetUp()
    {
        mLogfile.reset(new GMockLogfile());
        mLogManager.BeginLogging(mLogfile);
    }

    void TearDown()
    {
        mLogManager.EndLogging();
    }

    GMockLogfile& getLogfile()
    {
        return *mLogfile;
    }

    void setLogMask(int mask)
    {
        mLogManager.SetGlobalLogMask(mask);
    }

    GMockLogfilePtr mLogfile;
    LogManager mLogManager;
};

class LogMsgMatcher : public MatcherInterface<const Forte::LogMsg&>
{
public:
    explicit LogMsgMatcher(const Forte::FString& match)
      : mMatch(match)
    {
    }

    virtual bool MatchAndExplain(const Forte::LogMsg& msg,
                                 MatchResultListener* listener) const
    {
        return (msg.mMsg.find(mMatch) != FString::npos);
    }

    virtual void DescribeTo(::std::ostream* os) const
    {
        *os << " msg contain " << mMatch;
    }

    virtual void DescribeNegationTo(::std::ostream* os) const
    {
        *os << " msg does not contain " << mMatch;
    }

private:
    const Forte::FString mMatch;
};

inline Matcher<const Forte::LogMsg&> LogMsgMatch(const Forte::FString& match)
{
    return MakeMatcher(new LogMsgMatcher(match));
}

TEST_F(LoggingTest, trivial)
{
}

TEST_F(LoggingTest, ftrace_stream_level_all)
{
    setLogMask(HLOG_ALL);
    EXPECT_CALL(getLogfile(), Write(LogMsgMatch("(scale)"))).Times(2);
    FTRACESTREAM("scale");
}

TEST_F(LoggingTest, ftrace_level_trace)
{
    setLogMask(HLOG_TRACE);
    EXPECT_CALL(getLogfile(), Write(LogMsgMatch(""))).Times(2);
    FTRACE;
}

TEST_F(LoggingTest, ftrace_stream_level_trace)
{
    setLogMask(HLOG_TRACE);
    EXPECT_CALL(getLogfile(), Write(LogMsgMatch("(scale2050)"))).Times(2);
    FTRACESTREAM("s" << "c" << "a" << "l" << "e" << 2050);
}

TEST_F(LoggingTest, ftrace2_level_trace)
{
    setLogMask(HLOG_TRACE);
    EXPECT_CALL(getLogfile(), Write(LogMsgMatch("(scale2050)"))).Times(2);
    FTRACE2("%s%d", "scale", 2050);
}

TEST_F(LoggingTest, ftrace_level_debug)
{
    setLogMask(HLOG_DEBUG);
    EXPECT_CALL(getLogfile(), Write(_)).Times(0);
    FTRACE;
}

TEST_F(LoggingTest, ftrace_stream_level_debug)
{
    setLogMask(HLOG_DEBUG);
    EXPECT_CALL(getLogfile(), Write(_)).Times(0);
    FTRACESTREAM("s" << "c" << "a" << "l" << "e");
}

TEST_F(LoggingTest, ftrace2_level_debug)
{
    setLogMask(HLOG_DEBUG);
    EXPECT_CALL(getLogfile(), Write(_)).Times(0);
    FTRACE2("%s", "scale");
}

TEST_F(LoggingTest, hlog_stream_info_level_info)
{
    setLogMask(HLOG_INFO);
    EXPECT_CALL(getLogfile(), Write(LogMsgMatch("scale"))).Times(1);
    hlogstream(HLOG_INFO, "scale");
}

TEST_F(LoggingTest, hlog_stream_info_level_warn)
{
    setLogMask(HLOG_WARN);
    EXPECT_CALL(getLogfile(), Write(_)).Times(0);
    hlogstream(HLOG_INFO, "s" << "c" << "a" << "l" << "e");
}

TEST_F(LoggingTest, hlog_stream_set_level_inflight)
{
    setLogMask(HLOG_INFO);
    EXPECT_CALL(getLogfile(), Write(LogMsgMatch("scaleinfo"))).Times(1);
    hlogstream(HLOG_INFO, "scale" << "info");
    setLogMask(HLOG_WARN);
    hlogstream(HLOG_INFO, "scale" << "warn");
}
