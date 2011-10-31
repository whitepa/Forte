#include "DbMirroredConnection.h"
#include "DbConnectionFactory.h"
#include "DbSqlStatement.h"
#include "LogManager.h"
#include "FileSystem.h"
#include <sys/inotify.h>
#include <sqlite3.h> //todo: remove after add exceptions to sqliteconnection
#include <boost/shared_ptr.hpp>
#include <stdio.h>

using namespace Forte;
using namespace boost;

DbMirroredConnection::DbMirroredConnection(shared_ptr<DbConnectionFactory> primaryDbConnectionFactory,
                                           shared_ptr<DbConnectionFactory> secondaryDbConnectionFactory,
                                           const FString& altDbName)
    : mPrimaryDbConnectionFactory(primaryDbConnectionFactory),
      mSecondaryDbConnectionFactory(secondaryDbConnectionFactory),
      mDbConnection(primaryDbConnectionFactory->create()),
      mAltDbName(altDbName)
{
}

bool DbMirroredConnection::isActiveSecondary() const
{
    return (mDbConnection == mDbConnectionSecondary);
}

bool DbMirroredConnection::isActivePrimary() const
{
    return ! isActiveSecondary();
}

bool DbMirroredConnection::setActiveSecondary()
{
    try
    {
        AutoUnlockMutex guard(mMutex);

        if(mDbConnection->HasPendingQueries())
        {
            throw EDbConnectionPendingFailed();
        }

        if(! mDbConnectionSecondary)
        {
            hlog(HLOG_TRACE, "create a secondary DB connection");
            mDbConnectionSecondary.reset(mSecondaryDbConnectionFactory->create());
        }

        hlog(HLOG_TRACE, "try to init secondary DB connection to %s", mAltDbName.c_str());
        const bool ok(mDbConnectionSecondary->Init(mAltDbName, mDbConnection->mUser, mDbConnection->mPassword,
                      mDbConnection->mHost, mDbConnection->mSocket, 3));

        if(ok)
        {
            mDbConnection = mDbConnectionSecondary;
        }
        else
        {
            hlog(HLOG_TRACE, "failed to init secondary DB connection");
        }

        return ok;
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
        return false;
    }
    catch(EDbConnectionConnectFailed& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
        return false;
    }

    return false;
}

bool DbMirroredConnection::Init(const FString& db, const FString& user, const FString& pass, const FString& host, const FString& socket, unsigned int retries)
{
    AutoUnlockMutex guard(mMutex);

    if(isActiveSecondary())
    {
        hlog(HLOG_TRACE, "try to init secondary connection");

        try
        {
            const bool ret(mDbConnection->Init(mAltDbName, user, pass, host, socket, retries));
            return ret;
        }
        catch(EDbConnectionConnectFailed& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }
        catch(EDbConnectionIoError& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }
    }
    else
    {
        hlog(HLOG_TRACE, "attempt init primary DB connection");

        try
        {
            const bool ret(mDbConnection->Init(db, user, pass, host, socket, retries));
            if(ret)
            {
                FileSystem fs;
                if(! mAltDbName.empty() && ! fs.FileExists(mAltDbName))
                {
                    const string dirName(fs.Dirname(mAltDbName));
                    if(! fs.FileExists(dirName))
                    {
                        fs.MakeFullPath(dirName);
                    }

                    mDbConnection->BackupDatabase(mAltDbName);
                }
            }
            return ret;
        }
        catch(EDbConnectionConnectFailed& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }
        catch(EDbConnectionIoError& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }

        hlog(HLOG_TRACE, "failed to init primary DB Connection");

        return setActiveSecondary();
    }

    return false;
}

bool DbMirroredConnection::Connect()
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        return (mDbConnection->Connect());
    }
    catch(EDbConnectionConnectFailed& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            return Connect();
        }
    }

    return false;
}

bool DbMirroredConnection::Close()
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        return mDbConnection->Close();
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            return Close();
        }
    }

    return false;
}

bool DbMirroredConnection::Execute(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        return mDbConnection->Execute(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            return Execute(sql);
        }
    }

    return false;
}

DbResult DbMirroredConnection::Store(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        return mDbConnection->Store(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            return Store(sql);
        }
    }

    return DbResult();
}

DbResult DbMirroredConnection::Use(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        return mDbConnection->Use(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            return Use(sql);
        }
    }

    return DbResult();
}

bool DbMirroredConnection::Execute2(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    try
    {
      return mDbConnection->Execute2(sql);
    }
    catch(EDbConnectionIoError& e)
    {
      hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
          return Execute2(sql);
        }
    }

    return false;
}

DbResult DbMirroredConnection::Store2(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        return mDbConnection->Store2(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            return Store2(sql);
        }
    }

    return DbResult();
}

DbResult DbMirroredConnection::Use2(const FString& sql)
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        return mDbConnection->Use2(sql);
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            return Use2(sql);
        }
    }

    return DbResult();
}

void DbMirroredConnection::AutoCommit(bool enabled)
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        mDbConnection->AutoCommit(enabled);
        return;
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            AutoCommit(enabled);
        }
    }
}

void DbMirroredConnection::Begin()
{
    AutoUnlockMutex guard(mMutex);

    try
    {
        mDbConnection->Begin();
        return;
    }
    catch(EDbConnectionIoError& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }

    if(isActivePrimary())
    {
        if(setActiveSecondary())
        {
            Begin();
        }
    }
}

void DbMirroredConnection::Commit()
{
    AutoUnlockMutex guard(mMutex);

    if(isActiveSecondary())
    {
        throw EDbConnectionReadOnly();
    }
    else
    {
        try
        {
            mDbConnection->Commit();
            return;
        }
        catch(EDbConnectionIoError& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }

        if(setActiveSecondary())
        {
            return Commit();
        }
    }
}

void DbMirroredConnection::Rollback()
{
    AutoUnlockMutex guard(mMutex);

    if(isActiveSecondary())
    {
        throw EDbConnectionReadOnly();
    }
    else
    {
        try
        {
            mDbConnection->Rollback();
            return;
        }
        catch(EDbConnectionIoError& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }

        if(setActiveSecondary())
        {
            Rollback();
        }
    }
}

uint64_t DbMirroredConnection::InsertID()
{
    AutoUnlockMutex guard(mMutex);

    return mDbConnection->InsertID();
}

uint64_t DbMirroredConnection::AffectedRows()
{
    AutoUnlockMutex guard(mMutex);
    return mDbConnection->AffectedRows();
}

FString DbMirroredConnection::Escape(const char *str)
{
    AutoUnlockMutex guard(mMutex);
    return mDbConnection->Escape(str);
}

bool DbMirroredConnection::IsTemporaryError() const
{
    return mDbConnection->IsTemporaryError();
}

void DbMirroredConnection::BackupDatabase(const FString &targetPath)
{
    AutoUnlockMutex guard(mMutex);

    if(isActivePrimary())
    {
        mDbConnection->BackupDatabase(targetPath);
    }
}

bool DbMirroredConnection::Execute(const DbSqlStatement& statement)
{
    AutoUnlockMutex guard(mMutex);

    if(isActiveSecondary())
    {
        if(statement.IsMutator())
        {
            throw EDbConnectionReadOnly();
        }
        else
        {
            return mDbConnection->Execute(statement);
        }
    }
    else
    {
        try
        {
            return (mDbConnection->Execute(statement));
        }
        catch(EDbConnectionIoError& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }

        if(setActiveSecondary())
        {
            return Execute(statement);
        }
    }

    return false;
}

DbResult DbMirroredConnection::Use(const DbSqlStatement& statement)
{
    AutoUnlockMutex guard(mMutex);

    if(isActiveSecondary())
    {
        if(statement.IsMutator())
        {
            throw EDbConnectionReadOnly();
        }
        else
        {
            return mDbConnection->Use(statement);
        }
    }
    else
    {
        try
        {
            return (mDbConnection->Use(statement));
        }
        catch(EDbConnectionIoError& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }

        if(setActiveSecondary())
        {
            return Use(statement);
        }
    }

    return DbResult();
}

DbResult DbMirroredConnection::Store(const DbSqlStatement& statement)
{
    AutoUnlockMutex guard(mMutex);

    if(isActiveSecondary())
    {
        if(statement.IsMutator())
        {
            throw EDbConnectionReadOnly();
        }
        else
        {
            return mDbConnection->Store(statement);
        }
    }
    else
    {
        try
        {
            return (mDbConnection->Store(statement));
        }
        catch(EDbConnectionIoError& e)
        {
            hlog(HLOG_WARN, "%s", e.what());
        }

        if(setActiveSecondary())
        {
            return Store(statement);
        }
    }

    return DbResult();
}


DbMirroredConnection::~DbMirroredConnection()
{
}

const std::string& DbMirroredConnection::GetDbName() const
{
    return mDbConnection->GetDbName();
}
