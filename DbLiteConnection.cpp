#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_SQLITE

#include <boost/bind.hpp>
#include "DbLiteConnection.h"
#include "DbLiteResult.h"
#include "DbException.h"
#include "LogManager.h"
#include "FileSystemImpl.h"
#include "Util.h"
#include "FTrace.h"
#include "Clock.h"
#include "DbConnectionPool.h"

using namespace Forte;

DbLiteConnection::DbLiteConnection(int openFlags, const char *vfsName)
:
    DbConnection(),
    mFlags(openFlags),
    mVFSName (vfsName)
{
    FTRACE2("%i", openFlags);

    mDBType = "sqlite";
    mDB = NULL;
}


DbLiteConnection::~DbLiteConnection()
{
    FTRACE;
    if (!closeNonVirtual())
    {
        hlog(HLOG_ERR, "[%s] [%p] Failed to close (%i - %s)",
             mDBName.c_str(), this, mErrno, mError.c_str());
    }
}


void DbLiteConnection::setError()
{
    if (mDB != NULL)
    {
        mErrno = sqlite3_errcode(mDB);
        mError = sqlite3_errmsg(mDB);
    }
    else
    {
        mErrno = SQLITE_ERROR;
        mError = "Connection is gone.";
    }
}


bool DbLiteConnection::Init(const FString& db,
                            const FString& user,
                            const FString& pass,
                            const FString& host,
                            const FString& socket,
                            unsigned int retries)
{
    FTRACE2("%s,%s,%s,%s,%s,%u", db.c_str(), user.c_str(), pass.c_str(),
            host.c_str(), socket.c_str(), retries);
    return DbConnection::Init(db, user, pass, host, socket, retries);
}

bool DbLiteConnection::Init(const FString& dbPath,
                            unsigned int retries)
{
    return DbConnection::Init(dbPath, "", "", "", "", retries);
}


bool DbLiteConnection::Init(struct sqlite3 *db)
{
    FTRACE2("<db>");
    mDB = db;
    mDidInit = true;
    mReconnect = false;
    return true;
}


bool DbLiteConnection::Connect()
{
    FTRACE2("connecting to %s", mDBName.c_str());

    int err;

    if (mDB != NULL && !Close()) return false;

    err = sqlite3_open_v2(
        mDBName.c_str(), &mDB, mFlags,
        mVFSName.empty() ? NULL : mVFSName.c_str());

    if (err == SQLITE_OK)
    {
        hlog(HLOG_DEBUG2, "[%s][%p] sqlite connection open",
             mDBName.c_str(), mDB);
        sqlite3_busy_timeout(mDB, 1000); // 1 second

        Execute("PRAGMA synchronous = 1;");
        return true;
    }

    if (mDB != NULL)
    {
        setError();
        throw EDbConnectionConnectFailed();
    }
    else
    {
        mErrno = SQLITE_NOMEM;
        mError = "Out of memory";
    }

    return false;
}

bool DbLiteConnection::Close()
{
    return closeNonVirtual();
}

bool DbLiteConnection::closeNonVirtual()
{
    hlog(HLOG_TRACE, "[%s] closing [%p/%p]", mDBName.c_str(), this, mDB);

    if (sqlite3_close(mDB) == SQLITE_OK)
    {
        hlog(HLOG_DEBUG2, "[%s] close complete [%p/%p]", mDBName.c_str(), this, mDB);
        mDB = NULL;
        return true;
    }

    setError();
    return false;
}


DbResult DbLiteConnection::Query(const FString& sql)
{
    FTRACE2("[%s] %s on [%p/%p]", mDBName.c_str(), sql.c_str(), this, mDB);

    unsigned int tries_remaining = mRetries + 1;
    struct timeval tv_start, tv_end;
    DbLiteResult res;
    sqlite3_stmt *stmt = NULL;
    FString remain = sql;
    const char *tail = NULL;

    // init
    mTries = 0;
    mErrno = SQLITE_OK;
    mError.clear();

    if (mDB == NULL)
    {
        setError();
        return res;
    }
    mCurrentQuery = sql;
    if (mAutoCommit == false) mQueriesPending = true;

    // while queries remain in the given sql, and we have some tries left...
    while (remain.length() && tries_remaining > 0)
    {
        // prepare statement
        hlog(HLOG_DEBUG2, "[%s] Preparing statement [%s]", mDBName.c_str(),
             sql.c_str());
        mErrno = sqlite3_prepare_v2(mDB, remain, remain.length(), &stmt, &tail);
        if (stmt == NULL)
        {
            mTries++;
            switch (mErrno)
            {
            //// soft failures, keep retrying:
            case SQLITE_BUSY:
            case SQLITE_LOCKED:

                hlog(HLOG_WARN, "[%s] sqlite3_prepare_v2 failed on [%s]. "
                     "Error was %d.  Try %d of %d.",
                     mDBName.c_str(), sql.c_str(), mErrno,
                     (mRetries - tries_remaining + 1),
                     mRetries);

                --tries_remaining;
                if (tries_remaining > 0)
                    usleep(50000); // sleep 50 milliseconds
                continue;

                //// hard failures, just fail immediately:
            default:
                hlog(HLOG_WARN, "[%s] sqlite3_prepare_v2 failed on [%s]. "
                     "Error was %d.  hard failure, will not retry.",
                     mDBName.c_str(), sql.c_str(), mErrno);

                tries_remaining = 0;
                break;
            }
            continue;
        }

        do
        {
            // run query
            mTries++;
            gettimeofday(&tv_start, NULL);
            hlog(HLOG_DEBUG2, "[%s] Loading query [%s]", mDBName.c_str(),
                 sql.c_str());
            mErrno = res.Load(stmt);
            gettimeofday(&tv_end, NULL);
            if (sDebugSql) LogSql(remain, tv_end - tv_start);

            // check for errors
            if (mErrno != SQLITE_OK)
            {
                res.Clear();

                switch (mErrno)
                {
                    //// soft failures, keep retrying:
                case SQLITE_BUSY:
                case SQLITE_LOCKED:
                    // according to sqlite logs, if in a transaction
                    // and a statement fails that is *not* commit
                    // you must rollback the entire transaction and start over
                    if (mInTransaction && (sql != "commit"))
                    {
                        hlog(HLOG_WARN, "[%s] Load failed on [%s]. "
                             "Error was %d. in a transaction, cannot retry",
                             mDBName.c_str(), sql.c_str(), mErrno);

                        tries_remaining=0;
                    }
                    else
                    {
                        hlog(HLOG_WARN, "[%s] Load failed on [%s]. "
                             "Error was %d. Try %d of %d.",
                             mDBName.c_str(), sql.c_str(), mErrno,
                             (mRetries - tries_remaining + 1),
                             mRetries);

                        //sqlite3_reset(stmt);
                        --tries_remaining;
                        if (tries_remaining > 0)
                            usleep(50000); // sleep 50 milliseconds
                    }
                    break;

                    //// hard failures, just fail immediately:
                default:
                    hlog(HLOG_WARN, "[%s] Load failed on [%s]. "
                         "Error was %d. hard failure, cannot retry",
                         mDBName.c_str(), sql.c_str(), mErrno);

                    tries_remaining = 0;
                    break;
                }
            }
        }
        while (!res && tries_remaining > 0);

        // cleanup statement
        int rc;
        if ((rc = sqlite3_finalize(stmt)) != SQLITE_OK)
        {
            hlog(HLOG_WARN, "sqlite3_finalize unhandled error: %d", rc);
        }
        stmt = NULL;

        // set remaining sql
        if (tail == NULL) remain.clear();
        else remain = tail;
    }

    if(mErrno == SQLITE_IOERR)
    {
        throw EDbConnectionIoError();
    }

    if (!res)
        setError();

    return res;
}

bool DbLiteConnection::Execute(const FString& sql)
{
    return Query(sql);
}


DbResult DbLiteConnection::Store(const FString& sql)
{
    return Query(sql);
}


DbResult DbLiteConnection::Use(const FString& sql)
{
    return Query(sql);
}


bool DbLiteConnection::IsTemporaryError() const
{
    return (mErrno == SQLITE_BUSY || mErrno == SQLITE_LOCKED);
}


FString DbLiteConnection::Escape(const char *str)
{
    FString ret;
    char *tmp;

    if ((tmp = sqlite3_mprintf("%q", str)) != NULL)
    {
        ret = tmp;
        sqlite3_free(tmp);
    }

    return ret;
}

void DbLiteConnection::BackupDatabase(const FString &targetPath)
{
    hlog(HLOG_TRACE, "backing up db to %s", targetPath.c_str());

    if(mDBName == targetPath)
    {
        hlog(HLOG_ERROR, "- operation ignored, source and target are same path={%s}", mDBName.c_str());
        throw EDbLiteBackupFailedInvalidPath();
    }

    DbLiteConnection dbTarget(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    const FString tmpTargetPath(getTmpBackupPath(targetPath));
    dbTarget.Init(tmpTargetPath);

    backupDatabase(dbTarget);
}

void DbLiteConnection::BackupDatabase(DbConnection &dbTarget)
{
    FTRACE;
    backupDatabase(dynamic_cast<DbLiteConnection&>(dbTarget));
}


FString DbLiteConnection::getTmpBackupPath(const FString& targetPath) const
{
    return targetPath;
}

void DbLiteConnection::backupDatabase(DbLiteConnection &dbTarget)
{
    FTRACE2("Backing up (%s) %s -> (%s) %s", mDBType.c_str(), mDBName.c_str(),
            dbTarget.mDBType.c_str(), dbTarget.mDBName.c_str());

    /* Open the sqlite3_backup object used to accomplish the transfer */
    sqlite3_backup *pBackup;
    pBackup = sqlite3_backup_init(dbTarget.mDB, "main", mDB, "main");
    if (!pBackup)
    {
        hlog_and_throw(HLOG_WARN, EDbLiteBackupFailedSqlite3BackupInit());
    }

    int rc;
    do {
        rc = sqlite3_backup_step(pBackup, -1);
    } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

    /* Release resources allocated by backup_init(). */
    if(sqlite3_backup_finish(pBackup) != SQLITE_OK)
    {
        hlog(HLOG_ERROR, "failed to finish backup to %s",
             dbTarget.mDBName.c_str());
    }

    if(rc != SQLITE_DONE)
    {
        hlog_and_throw(HLOG_WARN, EDbLiteBackupFailedSqlite3BackupStep());
    }
}

uint64_t DbLiteConnection::InsertID()
{
    return sqlite3_last_insert_rowid(mDB);
}

uint64_t DbLiteConnection::AffectedRows()
{
    return sqlite3_changes(mDB);
}

#endif
#endif
