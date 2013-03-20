#ifndef DbUtil_h
#define DbUtil_h

#ifndef FORTE_NO_DB

#include "DbRow.h"
#include "DbResult.h"
#include "DbConnection.h"
#include "DbException.h"
#include "DbSqlStatement.h"
#include "LogManager.h"
#include <stdarg.h>

namespace Forte
{
    class DbUtil {
    public:

        static inline DbResult DbStore(const char *func, DbConnection &db, const char* sql)
        {
            hlog(HLOG_SQL, "[%s] Executing sql [%s] on [%p]",
                 db.GetDbName().c_str(), sql, (&db));

            DbResult result;
            if (!(result = db.Store(sql)))
            {
                // database error
                FString err;
                err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                           func, db.GetErrno(), db.GetTries(), db.GetRetries(), db.GetError().c_str(), sql);
                if (db.IsTemporaryError())
                    throw DbTempErrorException(err, db.GetErrno());
                else
                    throw DbException(err, db.GetErrno());
            }
            if (db.GetTries() > 1)
                hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                     func, db.GetTries(), db.GetErrno());
            return result;
        }

        static inline DbResult DbUse(const char *func, DbConnection &db, const char* sql)
        {
            hlog(HLOG_SQL, "[%s] Executing sql [%s] on [%p]",
                 db.GetDbName().c_str(), sql, (&db));
            DbResult result;
            if (!(result = db.Use(sql)))
            {
                // database error
                FString err;
                err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                           func, db.GetErrno(), db.GetTries(), db.GetRetries(), db.GetError().c_str(), sql);
                if (db.IsTemporaryError())
                    throw DbTempErrorException(err, db.GetErrno()); // indicate temporary error
                else
                    throw DbException(err, db.GetErrno());
            }
            if (db.GetTries() > 1)
                hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                     func, db.GetTries(), db.GetErrno());
            return result;
        }

        static inline void DbExecute(const char *func, DbConnection &db, const char* sql)
        {
            hlog(HLOG_SQL, "[%s] Executing sql [%s] on [%p]",
                 db.GetDbName().c_str(), sql, (&db));

            if (!db.Execute(sql))
            {
                // database error
                FString err;
                err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                           func, db.GetErrno(), db.GetTries(), db.GetRetries(), db.GetError().c_str(), sql);
                if (db.IsTemporaryError())
                    throw DbTempErrorException(err, db.GetErrno());
                else
                    throw DbException(err, db.GetErrno());
            }
            if (db.GetTries() > 1)
                hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                     func, db.GetTries(), db.GetErrno());
        }

        static inline DbResult DbStore(const char *func, DbConnection &db, const DbSqlStatement& sql)
        {
            hlog(HLOG_SQL, "[%s] Executing sql [%s] on [%p]",
                 db.GetDbName().c_str(),
                 sql.GetStatement().c_str(),
                 (&db));

            DbResult result;
            if (!(result = db.Store(sql)))
            {
                // database error
                FString err;
                err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                           func, db.GetErrno(), db.GetTries(), db.GetRetries(), db.GetError().c_str(), sql.GetStatement().c_str());
                if (db.IsTemporaryError())
                    throw DbTempErrorException(err, db.GetErrno());
                else
                    throw DbException(err, db.GetErrno());
            }
            if (db.GetTries() > 1)
                hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                     func, db.GetTries(), db.GetErrno());
            return result;
        }

        static inline DbResult DbUse(const char *func, DbConnection &db, const DbSqlStatement& sql)
        {
            hlog(HLOG_SQL, "[%s] Executing sql [%s] on [%p]",
                 db.GetDbName().c_str(),
                 sql.GetStatement().c_str(),
                 (&db));

            DbResult result;
            if (!(result = db.Use(sql)))
            {
                // database error
                FString err;
                err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                           func, db.GetErrno(), db.GetTries(), db.GetRetries(), db.GetError().c_str(), sql.GetStatement().c_str());
                if (db.IsTemporaryError())
                    throw DbTempErrorException(err, db.GetErrno()); // indicate temporary error
                else
                    throw DbException(err, db.GetErrno());
            }
            if (db.GetTries() > 1)
                hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                     func, db.GetTries(), db.GetErrno());
            return result;
        }

        static inline void DbExecute(const char *func, DbConnection &db, const DbSqlStatement& sql)
        {
            hlog(HLOG_SQL, "[%s] Executing sql [%s] on [%p]",
                 db.GetDbName().c_str(),
                 sql.GetStatement().c_str(),
                 (&db));

            if (!db.Execute(sql))
            {
                // database error
                FString err;
                err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                           func, db.GetErrno(), db.GetTries(), db.GetRetries(), db.GetError().c_str(), sql.GetStatement().c_str());
                if (db.IsTemporaryError())
                    throw DbTempErrorException(err, db.GetErrno());
                else
                    throw DbException(err, db.GetErrno());
            }
            if (db.GetTries() > 1)
                hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                     func, db.GetTries(), db.GetErrno());
        }

        static inline FString DbEscape(DbConnection &db, const char *sql_in)
        {
            return db.Escape(sql_in);
        }
    };
};
#define DbStore(db, sql) DbUtil::DbStore(__FUNCTION__, db, sql)
#define DbUse(db, sql) DbUtil::DbUse(__FUNCTION__, db, sql)
#define DbExecute(db, sql) DbUtil::DbExecute(__FUNCTION__, db, sql)
#define DbEscape(db, sql) DbUtil::DbEscape(db, sql)
#endif
#endif
