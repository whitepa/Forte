#ifndef __DbLiteConnection_h
#define __DbLiteConnection_h

#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_SQLITE

#include <sqlite3.h>
#include "FString.h"
#include "DbConnection.h"
#include "DbException.h"


namespace Forte
{
    EXCEPTION_SUBCLASS(DbException, EDbLiteBackupFailed);
    EXCEPTION_SUBCLASS(EDbLiteBackupFailed, EDbLiteBackupFailedInvalidPath);
    EXCEPTION_SUBCLASS(EDbLiteBackupFailed, EDbLiteBackupFailedSqlite3BackupInit);
    EXCEPTION_SUBCLASS(EDbLiteBackupFailed, EDbLiteBackupFailedSqlite3BackupStep);

    class DbLiteConnection : public DbConnection
    {
    public:
        DbLiteConnection(int openFlags = SQLITE_OPEN_READWRITE,
                         const char *vfsName = NULL);
        virtual ~DbLiteConnection();

        // initialization
        virtual bool Init(const FString& db,
                          const FString& user,
                          const FString& pass,
                          const FString& host = "localhost",
                          const FString& socket = "",
                          unsigned int retries = 0);

        bool Init(const FString& dbPath,
                  unsigned int retries = 0);

        bool Init(struct sqlite3 *db);

        // connection management
        virtual bool Connect();
        virtual bool Close();

        // queries
        virtual bool Execute(const FString& sql);
        virtual DbResult Store(const FString& sql);
        virtual DbResult Use(const FString& sql);

        // error info
        virtual bool IsTemporaryError() const;

        // misc.
        virtual uint64_t InsertID();
        virtual uint64_t AffectedRows();
        virtual FString Escape(const char *str);

        // SQLite specific backup functionality
        virtual void BackupDatabase(const FString &targetPath);
        virtual void BackupDatabase(DbConnection &targetDatabase);

    private:
        FString getTmpBackupPath(const FString& targetPath) const;
        void backupDatabase(DbLiteConnection &dbTarget);
        void setError();
        DbResult Query(const FString& sql);
        struct sqlite3 *mDB;

     public:
        int mFlags;

    private:
        const char *mVFSName;
    };
};
#endif
#endif
#endif
