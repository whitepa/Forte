#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_SQLITE

#include <boost/bind.hpp>
#include "DbLiteConnection.h"
#include "DbLiteResult.h"
#include "DbException.h"
#include "LogManager.h"
#include "FileSystemImpl.h"
#include "Util.h"
#include "AutoDoUndo.h"
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
    if (!Close())
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
    hlog(HLOG_DEBUG, "connecting to %s", mDBName.c_str());

    int err;

    if (mDB != NULL && !Close()) return false;

    err = sqlite3_open_v2(mDBName, &mDB, mFlags, mVFSName);

    if (err == SQLITE_OK)
    {
        hlog(HLOG_DEBUG2, "[%s][%p] sqlite connection open",
             mDBName.c_str(), mDB);
        sqlite3_busy_timeout(mDB, 1000); // 1 second
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
        hlog(HLOG_DEBUG, "[%s] Preparing statement [%s]", mDBName.c_str(),
             sql.c_str());
        mErrno = sqlite3_prepare_v2(mDB, remain, remain.length(), &stmt, &tail);
        if (stmt == NULL)
        {
            hlog(HLOG_WARN, "[%s] sqlite3_prepare_v2 failed on [%s]. "
                 "Error was %d.  Try %d of %d.",
                 mDBName.c_str(), sql.c_str(), mErrno,
                 (mRetries - tries_remaining + 1),
                 mRetries);
            mTries++;
            switch (mErrno)
            {
            //// soft failures, keep retrying:
            case SQLITE_BUSY:
            case SQLITE_LOCKED:
                --tries_remaining;
                if (tries_remaining > 0)
                    usleep(50000); // sleep 50 milliseconds
                continue;

                //// hard failures, just fail immediately:
            default:
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
            hlog(HLOG_DEBUG, "[%s] Loading query [%s]", mDBName.c_str(),
                 sql.c_str());
            mErrno = res.Load(stmt);
            gettimeofday(&tv_end, NULL);
            if (sDebugSql) LogSql(remain, tv_end - tv_start);

            // check for errors
            if (mErrno != SQLITE_OK)
            {
                res.Clear();
                hlog(HLOG_WARN, "[%s] Load failed on [%s]. "
                 "Error was %d.  Try %d of %d.",
                 mDBName.c_str(), sql.c_str(), mErrno,
                     (mRetries - tries_remaining + 1),
                     mRetries);

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
                        hlog(HLOG_DEBUG,
                             "[%s] In a transaction. [%s] will be rolled back",
                             mDBName.c_str(), sql.c_str());
                        tries_remaining=0;
                    }
                    else
                    {
                        //sqlite3_reset(stmt);
                        --tries_remaining;
                        if (tries_remaining > 0)
                            usleep(50000); // sleep 50 milliseconds
                    }
                    break;

                    //// hard failures, just fail immediately:
                default:
                    tries_remaining = 0;
                    break;
                }
            }
        }
        while (!res && tries_remaining > 0);

        // cleanup statement
        sqlite3_finalize(stmt);
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
    FTRACE2("'%s' -> '%s'", mDBName.c_str(), targetPath.c_str());
    typedef AutoDoUndo<void, void> BackupOperation;
    backupDatabase(targetPath);
}

FString DbLiteConnection::getTmpBackupPath(const FString& targetPath) const
{
    return targetPath;
}

void DbLiteConnection::backupDatabase(const FString &targetPath)
{
    hlog(HLOG_TRACE, "backing up db to %s", targetPath.c_str());

    if(mDBName == targetPath)
    {
        hlog(HLOG_ERROR, "- operation ignored, source and target are same path={%s}", mDBName.c_str());
        throw EDbLiteBackupFailedInvalidPath();
    }

    // @TODO this function is a prototype
    // See: http://www.sqlite.org/backup.html
    DbLiteConnection dbTarget(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    const FString tmpTargetPath(getTmpBackupPath(targetPath));
    dbTarget.Init(tmpTargetPath);

    /* Open the sqlite3_backup object used to accomplish the transfer */
    sqlite3_backup *pBackup;
    pBackup = sqlite3_backup_init(dbTarget.mDB, "main", mDB, "main");
    if (!pBackup)
    {
        hlog_and_throw(HLOG_WARN, EDbLiteBackupFailedSqlite3BackupInit());
    }

    int rc;
    do {
        rc = sqlite3_backup_step(pBackup, 5);
    } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

    if(rc != SQLITE_DONE)
    {
        hlog_and_throw(HLOG_WARN, EDbLiteBackupFailedSqlite3BackupStep());
    }

    /* Release resources allocated by backup_init(). */
    if(sqlite3_backup_finish(pBackup) != SQLITE_OK)
    {
        hlog(HLOG_ERROR, "failed to finish backup to %s",
             tmpTargetPath.c_str());
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
