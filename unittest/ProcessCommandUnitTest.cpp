#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "ProcessCommand.h"
#include "GMockProcessManager.h"

using namespace Forte;
using namespace boost;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::ReturnRef;
using ::testing::AnyNumber;
using ::testing::StrEq;
using ::testing::SetArgReferee;
using ::testing::ContainsRegex;
using ::testing::Invoke;
using ::testing::InSequence;
using ::testing::Mock;


LogManager logManager;

class ProcessCommandTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
    };
    static void TearDownTestCase() {
    };

};

TEST_F(ProcessCommandTest, basic)
{
    typedef Forte::ProcessCommand<Forte::ProcessCommandRequestBasic, Forte::ProcessCommandResponseBasic>
            ProcessCommandEcho;

    boost::shared_ptr<GMockProcessManager> pm(new GMockProcessManager);

    static const char* ret("\n");
    EXPECT_CALL(*pm, CreateProcessAndGetResult(
                    ::testing::StrEq("echo"),
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_))
        .Times(1)
        .WillOnce(DoAll(::testing::SetArgReferee<1>(ret),
                      ::testing::Return(0)));

    ProcessCommandEcho echo(pm, "echo");
    ProcessCommandEcho::response_type response(echo());
    EXPECT_EQ("\n", response.AsString());
}


class LSProcessCommandRequest : public Forte::ProcessCommandRequest
{
public:
    LSProcessCommandRequest(const Forte::FString& path)
        :mLs("ls"), mRequest(mLs + " " + path)
    {
    }

    Forte::FString AsString() const
    {
        return mRequest;
    }

private:
    const Forte::FString mLs;
    const Forte::FString mRequest;
};

class LSProcessCommandResponse : public ProcessCommandResponse
{
public:
    LSProcessCommandResponse(const Forte::FString& output,
                             const Forte::FString& errorOutput,
                             int returnCode)
        :ProcessCommandResponse(output, errorOutput, returnCode)
    {
    }

    std::vector<FString> GetFileList() const
    {
        std::vector<FString> components;
        mResponse.Explode(" ", components);
        return components;
    }
};

TEST_F(ProcessCommandTest, ls)
{
    typedef ProcessCommand<LSProcessCommandRequest, LSProcessCommandResponse> LSProcessCommand;

    boost::shared_ptr<GMockProcessManager> pm(new GMockProcessManager);

    static const char* ret("a b c d e f g");

    EXPECT_CALL(*pm, CreateProcessAndGetResult(
                    ::testing::StrEq("ls /tmp"),
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_))
        .Times(1)
        .WillOnce(DoAll(::testing::SetArgReferee<1>(ret),
                      ::testing::Return(0)));

    LSProcessCommand qemuInfo(pm, "/tmp");
    LSProcessCommand::response_type response(qemuInfo());
    EXPECT_EQ(7, response.GetFileList().size());
    EXPECT_EQ("a", response.GetFileList()[0]);
    EXPECT_EQ("g", response.GetFileList()[6]);
}

TEST_F(ProcessCommandTest, lsWithNonZeroReturnCode)
{
    typedef ProcessCommand<LSProcessCommandRequest, LSProcessCommandResponse> LSProcessCommand;

    boost::shared_ptr<GMockProcessManager> pm(new GMockProcessManager);

    static const char* ret("a b c d e f g");

    EXPECT_CALL(*pm, CreateProcessAndGetResult(
                    ::testing::StrEq("ls /tmp"),
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_,
                    ::testing::_))
        .Times(1)
        .WillOnce(DoAll(::testing::SetArgReferee<1>(ret),
                      ::testing::Return(1)));

    LSProcessCommand qemuInfo(pm, "/tmp");
    LSProcessCommand::response_type response(qemuInfo());
    EXPECT_EQ(7, response.GetFileList().size());
    EXPECT_EQ("a", response.GetFileList()[0]);
    EXPECT_EQ("g", response.GetFileList()[6]);
}
