#ifndef FORTE_NO_DB
#ifndef FORTE_NO_SQLITE

#include <boost/bind.hpp>
#include "DbLiteConnection.h"
#include "DbLiteResult.h"
#include "DbException.h"
#include "LogManager.h"
#include "FileSystem.h"
#include "Util.h"
#include "AutoDoUndo.h"

using namespace Forte;

DbLiteConnection::DbLiteConnection(int openFlags)
:
    DbConnection(),
    mFlags(openFlags)
{
    mDBType = "sqlite";
    mDB = NULL;
}


DbLiteConnection::~DbLiteConnection()
{
    Close();
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
    return DbConnection::Init(db, user, pass, host, socket, retries);
}

bool DbLiteConnection::Init(const FString& dbPath, 
                            unsigned int retries)
{
    return DbConnection::Init(dbPath, "", "", "", "", retries);
}


bool DbLiteConnection::Init(struct sqlite3 *db)
{
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

    err = sqlite3_open_v2(mDBName, &mDB, mFlags, NULL);

    if (err == SQLITE_OK)
    {
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
    hlog(HLOG_TRACE, "closing %s", mDBName.c_str());

    if (sqlite3_close(mDB) == SQLITE_OK)
    {
        mDB = NULL;
        return true;
    }

    setError();
    return false;
}


DbResult DbLiteConnection::Query(const FString& sql)
{
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

    if (mAutoCommit == false) mQueriesPending = true;

    // while queries remain in the given sql, and we have some tries left...
    while (remain.length() && tries_remaining > 0)
    {
        // prepare statement
        mErrno = sqlite3_prepare_v2(mDB, remain, remain.length(), &stmt, &tail);
        if (stmt == NULL)
        {
            hlog(HLOG_INFO, "sqlite3_prepare_v2 failed Error was %d", mErrno);

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
                    sqlite3_reset(stmt);
                    --tries_remaining;
                    if (tries_remaining > 0)
                        usleep(50000); // sleep 50 milliseconds
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
    typedef AutoDoUndo<void, void> BackupOperation;
    BackupOperation op(boost::bind(&::DbLiteConnection::backupDatabase, this, targetPath),
                       boost::bind(&::DbLiteConnection::removeTmpBackup, this, targetPath));
}

void DbLiteConnection::removeTmpBackup(const FString &targetPath)
{
    try
    {
        FileSystem fs;
        const FString tmpTargetPath(getTmpBackupPath(targetPath));
        if(fs.FileExists(tmpTargetPath))
        {
            fs.Unlink(tmpTargetPath);
        }
    }
    catch(Exception& e)
    {
        hlog(HLOG_WARN, "%s", e.what());
    }
}

FString DbLiteConnection::getTmpBackupPath(const FString& targetPath) const
{
    FileSystem fs;
    FString tmpTargetPath(targetPath);
    tmpTargetPath += ".tmp";
    return tmpTargetPath;
}

void DbLiteConnection::backupDatabase(const FString &targetPath)
{
    hlog(HLOG_TRACE, "backing up db to %s", targetPath.c_str());

    if(mDBName == targetPath)
    {
        hlog(HLOG_ERROR, "- operation ignored, source and target are same path={%s}", mDBName.c_str());
        throw EDbLiteBackupFailed();
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
        throw EDbLiteBackupFailed();

    /* Each iteration of this loop copies 5 database pages from database
    ** pDb to the backup database. If the return value of backup_step()
    ** indicates that there are still further pages to copy, sleep for
    ** 250 ms before repeating. */
    int rc;
    do {
        rc = sqlite3_backup_step(pBackup, 5);
        // xProgress(
        //     sqlite3_backup_remaining(pBackup),
        //     sqlite3_backup_pagecount(pBackup)
        //     );
        // if( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED ){
        //     sqlite3_sleep(250);
        // }
    } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

    if(rc != SQLITE_DONE)
    {
        throw EDbLiteBackupFailed();
    }

    /* Release resources allocated by backup_init(). */
    if(sqlite3_backup_finish(pBackup) != SQLITE_OK)
    {
        hlog(HLOG_ERROR, "failed to finish backup to %s", tmpTargetPath.c_str());
    }

    FileSystem fs;
    fs.Rename(tmpTargetPath, targetPath);
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
