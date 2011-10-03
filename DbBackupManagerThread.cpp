#include "DbBackupManagerThread.h"
#include "DbConnectionPool.h"
#include "DbAutoConnection.h"
#include "FileSystem.h"
#include "INotify.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <sys/inotify.h>

using namespace boost;
using namespace Forte;

DbBackupManagerThread::DbBackupManagerThread(boost::shared_ptr<DbConnectionPool> pool)
    :base_type(), mPool(pool), mInotify(make_shared<INotify>())
{
    if(pool && ! pool->GetDbName().empty() && ! pool->GetBackupDbName().empty())
    {
        mInotify->AddWatch(pool->GetDbName(), IN_MODIFY | IN_ATTRIB);
        setThreadName("backupmgr");
        initialized();
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
    try
    {
        DbAutoConnection connection(mPool);
        connection->BackupDatabase(mPool->GetBackupDbName());
    }
    catch(DbException& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }
}

void* DbBackupManagerThread::run()
{
    try
    {
        DbAutoConnection connection(mPool);
    }
    catch(DbException& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    timespec previousTimeSpec(getModificationTime());

    while(! this->IsShuttingDown())
    {
        INotify::EventVector events (mInotify->Read(TIMEOUT_SECS_BEFORE_CHECKING_MODIFICATION_TIME));

        if(events.size())
        {
            hlog(HLOG_TRACE, "%lu kicks to be serviced", events.size());

            backupDb();
            previousTimeSpec = getModificationTime();
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
    FileSystem fs;
    mInotify->Interrupt();
}
