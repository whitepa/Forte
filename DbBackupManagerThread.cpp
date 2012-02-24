#include "DbBackupManagerThread.h"
#include "DbConnectionPool.h"
#include "DbAutoConnection.h"
#include "FileSystemImpl.h"
#include "INotify.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <sys/inotify.h>
#include "FTrace.h"

#include <boost/filesystem/path.hpp>

using namespace boost;
using namespace Forte;

DbBackupManagerThread::DbBackupManagerThread(boost::shared_ptr<DbConnectionPool> pool)
    :base_type(), mPool(pool), mDbWatch(-1), mDbParentDirWatch(-1)
{
    if(pool && ! pool->GetDbName().empty() && ! pool->GetBackupDbName().empty())
    {
        mInotify.reset(new INotify());

        addWatchDbParentDir();
        addWatchDb();

        setThreadName("backupmgr");
        initialized();
    }
}

void DbBackupManagerThread::addWatchDbParentDir()
{
    FileSystemImpl fs;
    const string parentPath(fs.Dirname(mPool->GetDbName()));

    try
    {
        if(fs.FileExists(parentPath))
        {
            mDbParentDirWatch = mInotify->AddWatch(parentPath, IN_CREATE | IN_IGNORED);
            hlogstream(HLOG_DEBUG, "added watch on: " << parentPath);
        }
    }
    catch(EINotifyAddWatchFailed& e)
    {
        hlogstream(HLOG_WARN, "no primary db to watch during ctor: " << e.what());
    }
}

void DbBackupManagerThread::addWatchDb()
{
    const string& dbPath(mPool->GetDbName());

    try
    {
        FileSystemImpl fs;
        if(fs.FileExists(dbPath))
        {
            mDbWatch = mInotify->AddWatch(dbPath, IN_MODIFY | IN_ATTRIB);
            hlogstream(HLOG_DEBUG, "added watch on: " << dbPath);
        }
    }
    catch(EINotifyAddWatchFailed& e)
    {
        hlogstream(HLOG_WARN, "no parent path to db to watch during ctor: " << e.what());
    }
}

DbBackupManagerThread::~DbBackupManagerThread()
{
    deleting();
}

timespec DbBackupManagerThread::getModificationTime() const
{
    struct stat status;

    if(stat(mPool->GetDbName().c_str(), &status)==0)
    {
        return status.st_mtim;
    }
    else
    {
        struct timespec timeSpec;
        std::memset(&timeSpec, 0, sizeof(timeSpec));
        return timeSpec;
    }
}

void DbBackupManagerThread::backupDb()
{
    FTRACE;
    try
    {
        DbAutoConnection connection(mPool);
        connection->BackupDatabase(mPool->GetBackupDbName());
    }
    catch(DbException& e)
    {
        hlogstream(HLOG_DEBUG, e.what());
    }
    catch(std::exception& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }
}

void DbBackupManagerThread::resetWatchDbParentDir()
{
    FileSystemImpl fs;
    hlogstream(HLOG_DEBUG, "watch removed for parent" << fs.Dirname(mPool->GetDbName()));
    mDbParentDirWatch = -1;
}

void DbBackupManagerThread::resetWatchDb()
{
    hlogstream(HLOG_DEBUG, "watch removed for db: " << mPool->GetDbName());
    mDbWatch = -1;
}

bool DbBackupManagerThread::hasWatchDbParentDir() const
{
    return (mDbParentDirWatch != -1);
}

bool DbBackupManagerThread::hasWatchDb() const
{
    return (mDbWatch != -1);
}

void* DbBackupManagerThread::run()
{
    FileSystemImpl fs;

    backupDb();

    timespec previousTimeSpec(getModificationTime());

    while(! this->IsShuttingDown())
    {
        if(! hasWatchDbParentDir())
        {
            addWatchDbParentDir();
        }

        INotify::EventVector events (mInotify->Read(TIMEOUT_MS_BEFORE_CHECKING_MODIFICATION_TIME));

        if(! events.empty())
        {
            INotify::EventVector::size_type kickCount(0);

            for(INotify::EventVector::const_iterator i=events.begin(); i!=events.end(); i++)
            {
                const INotify::Event& event(**i);

                hlogstream(HLOG_DEBUG, event);

                if(event.wd == mDbParentDirWatch)
                {
                    if(event.mask & IN_IGNORED)
                    {
                        resetWatchDbParentDir();
                    }
                    else if(event.mask & IN_CREATE)
                    {
                        const string dbName(fs.Basename(mPool->GetDbName()));

                        if(event.name == dbName)
                        {
                            addWatchDb();
                        }
                    }
                    else
                    {
                        hlogstream(HLOG_DEBUG, "unhandled " << event);
                    }
                }
                else if(event.wd == mDbWatch)
                {
                    if (event.mask & IN_IGNORED)
                    {
                        kickCount = 0;
                        resetWatchDb();
                    }
                    else if(event.mask & (IN_MODIFY | IN_ATTRIB))
                    {
                        ++kickCount;
                    }
                    else
                    {
                        hlogstream(HLOG_DEBUG, "unhandled " << event);
                    }
                }
            }

            if(kickCount > 0 && hasWatchDb())
            {
                hlogstream(HLOG_DEBUG, kickCount << " kicks to be serviced");
                backupDb();
                previousTimeSpec = getModificationTime();
            }
        }
        else
        {
            timespec currentTimeSpec(getModificationTime());

            if((currentTimeSpec.tv_sec != previousTimeSpec.tv_sec) || (currentTimeSpec.tv_nsec != previousTimeSpec.tv_nsec))
            {
                backupDb();
                previousTimeSpec = currentTimeSpec;
            }
        }
    }

    return 0;
}


void DbBackupManagerThread::Shutdown()
{
    Thread::Shutdown();
    mInotify->Interrupt();
}
