#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sys/inotify.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/convenience.hpp>
#include "LogManager.h"
#include "INotify.h"

#include "Forte.h"
#include "Context.h"
#include "FileSystemImpl.h"

// #SCQAD TEST: ONBOX: INotifyUnitTest

// #SCQAD TESTTAG: smoketest, forte

using namespace Forte;

class INotifyTest : public ::testing::Test
{
protected:
    INotifyTest(): ::testing::Test(), watchedFd(-1), mTouched(false)
    {
    }

    string getWatchedFileName() const
    {
        return "deleteme";
    }

    string getWatchedFileDir() const
    {
        return "/fsscale0";
    }

    string getWatchedFilePath() const
    {
        return getWatchedFileDir() + "/" + getWatchedFileName();
    }

    bool existsWatchedFileDir() const
    {
        FileSystemImpl fs;
        return fs.FileExists(getWatchedFileDir());
    }

    virtual void SetUp()
    {
        mLogManager.BeginLogging("//stderr");
        ASSERT_TRUE(existsWatchedFileDir()) << getWatchedFileDir();
    }

    void addWatch(const std::string& filePath)
    {
        watchedFd = mInotify.AddWatch(filePath, IN_ATTRIB);
    }

    void removeWatch(int fd)
    {
        mInotify.RemoveWatch(fd);
    }

    void touch()
    {
        FileSystemImpl fs;
        fs.Touch(getWatchedFilePath());
        mTouched = true;
    }

    virtual void TearDown()
    {
        if(mTouched)
        {
            FileSystemImpl fs;
            fs.Unlink(getWatchedFilePath());
        }
    }

protected:
    LogManager mLogManager;
    INotify mInotify;
    int watchedFd;
    bool mTouched;
};


TEST_F(INotifyTest, INotifyTestBasicFileModification)
{
    touch();
    addWatch(getWatchedFilePath());
    touch();

    const INotify::EventVector events (mInotify.Read(0));

    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0]->wd, watchedFd);
}


TEST_F(INotifyTest, INotifyTestThrowsEINotifyAddWatchFailed)
{
    touch();
    ASSERT_THROW(addWatch("/tmp/asdlfkjasdlfjasldfkjaldsfkj"), Forte::EINotifyAddWatchFailed);
    touch();
}

TEST_F(INotifyTest, INotifyTestThrowsEINotifyRemoveWatchFailed)
{
    touch();
    addWatch(getWatchedFilePath());
    touch();

    const INotify::EventVector events (mInotify.Read(1));
    ASSERT_THROW(removeWatch(watchedFd+1), Forte::EINotifyRemoveWatchFailed);
}
