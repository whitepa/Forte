#include "DbMirroredConnectionUseSecondary.h"
#include "DbConnectionFactory.h"
#include "DbSqlStatement.h"
#include "LogManager.h"
#include "FileSystemImpl.h"
#include <sys/inotify.h>
#include <sqlite3.h> //todo: remove after add exceptions to sqliteconnection
#include <boost/shared_ptr.hpp>
#include <stdio.h>

using namespace Forte;
using namespace boost;

DbMirroredConnectionUseSecondary::DbMirroredConnectionUseSecondary(
    boost::shared_ptr<DbConnectionFactory> primaryDbConnectionFactory,
    boost::shared_ptr<DbConnectionFactory> secondaryDbConnectionFactory,
    const FString& altDbName)
    : mRWDbConnectionFactory(primaryDbConnectionFactory),
      mROnlyDbConnectionFactory(secondaryDbConnectionFactory),
      mDbConnectionReadOnly(secondaryDbConnectionFactory->create()),
      mDbConnectionReadWrite(primaryDbConnectionFactory->create()),
      mAltDbName(altDbName)
{
}

bool DbMirroredConnectionUseSecondary::setReadOnlyActive()
{
    if (mReadOnlyDbActive) return true;

    try
    {
        AutoUnlockMutex guard(mMutex);

        if (mDbConnectionReadWrite->HasPendingQueries())
        {
            throw EDbConnectionPendingFailed();
        }

        const bool ok(mDbConnectionReadOnly->Init(
                          mAltDbName,
                          mDbConnectionReadWrite->mUser,
                          mDbConnectionReadWrite->mPassword,
                          mDbConnectionReadWrite->mHost,
                          mDbConnectionReadWrite->mSocket,
                          mDbConnectionReadWrite->GetRetries()));

        mDbConnectionReadWrite->Close();

        mReadOnlyDbActive = true;
        return ok;
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
        return false;
    }
    catch(EDbConnectionConnectFailed& e)
    {
        hlogstream(HLOG_WARN, e.what());
        return false;
    }

    return false;
}

bool DbMirroredConnectionUseSecondary::setReadWriteActive()
{
    if (!mReadOnlyDbActive) return true;

    try
    {
        AutoUnlockMutex guard(mMutex);

        if (mDbConnectionReadOnly->HasPendingQueries())
        {
            throw EDbConnectionPendingFailed();
        }

        const bool ok(mDbConnectionReadWrite->Init(
                          mDbName,
                          mDbConnectionReadOnly->mUser,
                          mDbConnectionReadOnly->mPassword,
                          mDbConnectionReadOnly->mHost,
                          mDbConnectionReadOnly->mSocket,
                          mDbConnectionReadOnly->GetRetries()));

        mDbConnectionReadOnly->Close();

        mReadOnlyDbActive = false;
        return ok;
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
        return false;
    }
    catch(EDbConnectionConnectFailed& e)
    {
        hlogstream(HLOG_WARN, e.what());
        return false;
    }

    return false;
}

bool DbMirroredConnectionUseSecondary::Init(const FString& db, const FString& user, const FString& pass, const FString& host, const FString& socket, unsigned int retries)
{
    AutoUnlockMutex guard(mMutex);

    mDbName = db;

    FileSystemImpl fs;
    if (!mAltDbName.empty() && !fs.FileExists(mAltDbName))
    {
        try
        {
            const bool ret(mDbConnectionReadWrite->Init(
                               db, user, pass, host, socket, retries));
            if (ret)
            {
                const string dirName(fs.Dirname(mAltDbName));
                if(! fs.FileExists(dirName))
                {
                    fs.MakeFullPath(dirName);
                }

                mDbConnectionReadWrite->BackupDatabase(mAltDbName);
            }

            mReadOnlyDbActive = false;
            return ret;
        }
        catch(EDbConnectionConnectFailed& e)
        {
            if (hlog_ratelimit(60))
                hlogstream(HLOG_WARN, e.what());
        }
        catch(EDbConnectionIoError& e)
        {
            if (hlog_ratelimit(60))
                hlogstream(HLOG_WARN, e.what());
        }
        if (hlog_ratelimit(60))
            hlog(HLOG_WARN, "failed to init primary DB Connection: '%s'",
                 db.c_str());
    }
    else
    {
        try
        {
            const bool ret(mDbConnectionReadOnly->Init(
                               mAltDbName, user, pass, host, socket, retries));

            mReadOnlyDbActive = true;
            return ret;
        }
        catch(EDbConnectionConnectFailed& e)
        {
            hlogstream(HLOG_WARN, e.what());
        }
        catch(EDbConnectionIoError& e)
        {
            hlogstream(HLOG_WARN, e.what());
        }
    }

    return false;
}

bool DbMirroredConnectionUseSecondary::Connect()
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        if (mReadOnlyDbActive)
            return (mDbConnectionReadOnly->Connect());
        else
            return (mDbConnectionReadWrite->Connect());
    }
    catch(EDbConnectionConnectFailed& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return false;
}

bool DbMirroredConnectionUseSecondary::Close()
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        if (mReadOnlyDbActive)
            return mDbConnectionReadOnly->Close();
        else
            return mDbConnectionReadWrite->Close();
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return false;
}

bool DbMirroredConnectionUseSecondary::Execute(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(sql);

    try
    {
        return dbConnection->Execute(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return false;
}

DbResult DbMirroredConnectionUseSecondary::Store(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(sql);

    try
    {
        return dbConnection->Store(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return DbResult();
}

DbResult DbMirroredConnectionUseSecondary::Use(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(sql);

    try
    {
        return dbConnection->Use(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return DbResult();
}

bool DbMirroredConnectionUseSecondary::Execute2(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(sql);

    try
    {
      return dbConnection->Execute2(sql);
    }
    catch(EDbConnectionIoError& e)
    {
      hlogstream(HLOG_WARN, e.what());
    }

    return false;
}


DbResult DbMirroredConnectionUseSecondary::Store2(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(sql);

    try
    {
        return dbConnection->Store2(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return DbResult();
}

DbResult DbMirroredConnectionUseSecondary::Use2(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(sql);

    try
    {
        return dbConnection->Use2(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return DbResult();
}

void DbMirroredConnectionUseSecondary::AutoCommit(bool enabled)
{
    AutoUnlockMutex guard(mMutex);

    if (!enabled)
        setReadWriteActive();

    auto dbConnection = selectDbToUse();

    try
    {
        dbConnection->AutoCommit(enabled);
        return;
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }
}

void DbMirroredConnectionUseSecondary::Begin()
{
    AutoUnlockMutex guard(mMutex);

    setReadWriteActive();

    auto dbConnection = selectDbToUse();

    try
    {
        dbConnection->Begin();
        return;
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }
}

void DbMirroredConnectionUseSecondary::Commit()
{
    AutoUnlockMutex guard(mMutex);

    setReadWriteActive();

    if(mReadOnlyDbActive)
    {
        // @TODO we don't always need to throw in this case, as it is
        // feasible the database user may have done several select
        // calls (reading only) in a transaction, and is calling
        // commit to end the transaction.  Let's think some more about
        // this before changing the behavior.
        throw EDbConnectionReadOnly();
    }
    else
    {

        auto dbConnection = selectDbToUse();
        try
        {
            dbConnection->Commit();
            return;
        }
        catch(EDbConnectionIoError& e)
        {
            hlogstream(HLOG_WARN, e.what());
        }

        throw EDbConnectionReadOnly();
    }
}

void DbMirroredConnectionUseSecondary::Rollback()
{
    AutoUnlockMutex guard(mMutex);

    if(mReadOnlyDbActive)
    {
        throw EDbConnectionReadOnly();
    }
    else
    {
        auto dbConnection = selectDbToUse();
        try
        {
            dbConnection->Rollback();
            return;
        }
        catch(EDbConnectionIoError& e)
        {
            hlogstream(HLOG_WARN, e.what());
        }

        throw EDbConnectionReadOnly();
    }
}

uint64_t DbMirroredConnectionUseSecondary::InsertID()
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse();
    try
    {
        return dbConnection->InsertID();
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_ERROR, e.what());

        throw;
    }
}

uint64_t DbMirroredConnectionUseSecondary::AffectedRows()
{
    auto dbConnection = selectDbToUse();
    try
    {
        return dbConnection->AffectedRows();
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_ERROR, e.what());

        throw;
    }
}

FString DbMirroredConnectionUseSecondary::Escape(const char *str)
{
    AutoUnlockMutex guard(mMutex);
    auto dbConnection = selectDbToUse();
    return dbConnection->Escape(str);
}

FString DbMirroredConnectionUseSecondary::GetError() const
{
    auto dbConnection = selectDbToUse();
    return dbConnection->GetError();
}
unsigned int DbMirroredConnectionUseSecondary::GetErrno() const
{
    auto dbConnection = selectDbToUse();
    return dbConnection->GetErrno();
}

unsigned int DbMirroredConnectionUseSecondary::GetTries() const
{
    auto dbConnection = selectDbToUse();
    return dbConnection->GetTries();
}

bool DbMirroredConnectionUseSecondary::IsTemporaryError() const
{
    auto dbConnection = selectDbToUse();
    return dbConnection->IsTemporaryError();
}

unsigned int DbMirroredConnectionUseSecondary::GetRetries() const
{
    auto dbConnection = selectDbToUse();
    return dbConnection->GetRetries();
}

unsigned int DbMirroredConnectionUseSecondary::GetQueryRetryDelay() const
{
    auto dbConnection = selectDbToUse();
    return dbConnection->GetQueryRetryDelay();
}

void DbMirroredConnectionUseSecondary::BackupDatabase(const FString &targetPath)
{
    AutoUnlockMutex guard(mMutex);

    if (mReadOnlyDbActive)
        setReadWriteActive();

    auto dbConnection = selectDbToUse();
    dbConnection->BackupDatabase(targetPath);
}

void DbMirroredConnectionUseSecondary::BackupDatabase(DbConnection &targetDatabase)
{
    AutoUnlockMutex guard(mMutex);

    if (mReadOnlyDbActive)
        setReadWriteActive();

    auto dbConnection = selectDbToUse();
    dbConnection->BackupDatabase(targetDatabase);
}
bool DbMirroredConnectionUseSecondary::Execute(const DbSqlStatement& statement)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(statement);

    try
    {
        return dbConnection->Execute(statement);
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return false;
}

DbResult DbMirroredConnectionUseSecondary::Use(const DbSqlStatement& statement)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(statement);

    try
    {
        return (dbConnection->Use(statement));
    }
    catch(EDbConnectionIoError& e)
    {
        hlogstream(HLOG_WARN, e.what());
    }

    return DbResult();
}

DbResult DbMirroredConnectionUseSecondary::Store(const DbSqlStatement& statement)
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse(statement);

    try
    {
        return (dbConnection->Store(statement));
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    return DbResult();
}


DbMirroredConnectionUseSecondary::~DbMirroredConnectionUseSecondary()
{
}

const std::string& DbMirroredConnectionUseSecondary::GetDbName() const
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse();

    return dbConnection->GetDbName();

}

bool DbMirroredConnectionUseSecondary::HasPendingQueries() const
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse();

    return dbConnection->HasPendingQueries();

}

const FString& DbMirroredConnectionUseSecondary::GetCurrentQuery() const
{
    AutoUnlockMutex guard(mMutex);

    auto dbConnection = selectDbToUse();

    return dbConnection->GetCurrentQuery();

}
