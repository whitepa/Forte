#ifndef DbUtil_h
#define DbUtil_h

#ifndef FORTE_NO_DB

#include "DbRow.h"
#include "DbResult.h"
#include "DbConnection.h"
#include "DbException.h"
#include "LogManager.h"
#include <stdarg.h>

class CDbUtil {
public:
    static inline CDbResult DbStore(const char *func, CDbConnection &db, const char *sql)
    {
#ifdef DB_DEBUG
        hlog(HLOG_DEBUG, "Executing sql [%s]", sql);
#else
        if (db.m_log_queries) hlog(HLOG_DEBUG, "Executing sql [%s]", sql);
#endif
        CDbResult result;
        if (!(result = db.store(sql)))
        {
            // database error
            FString err;
            err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                       func, db.m_errno, db.m_tries, db.m_retries, db.m_error.c_str(), sql);
            if (db.isTemporaryError())
                throw CDbTempErrorException(err, db.m_errno);
            else
                throw CDbException(err, db.m_errno);
        }
        if (db.m_tries > 1)
            hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                 func, db.m_tries, db.m_errno);
        return result;
    }
    static inline CDbResult DbUse(const char *func, CDbConnection &db, const char *sql)
    {
        if (db.m_log_queries) hlog(HLOG_DEBUG, "Executing sql [%s]", sql);
        CDbResult result;
        if (!(result = db.use(sql)))
        {
            // database error
            FString err;
            err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                       func, db.m_errno, db.m_tries, db.m_retries, db.m_error.c_str(), sql);
            if (db.isTemporaryError())
                throw CDbTempErrorException(err, db.m_errno); // indicate temporary error
            else
                throw CDbException(err, db.m_errno);
        }
        if (db.m_tries > 1)
            hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                 func, db.m_tries, db.m_errno);
        return result;
    }
    static inline void DbExecute(const char *func, CDbConnection &db, const char *sql)
    {
#ifdef DB_DEBUG
	hlog(HLOG_DEBUG, "Executing sql [%s]", sql);
#else
        if (db.m_log_queries) hlog(HLOG_DEBUG, "Executing sql [%s]", sql);
#endif
        if (!db.execute(sql))
        {
            // database error
            FString err;
            err.Format("%s: query failed; errno=%u tries=%u retries=%u error=%s full_query=%s",
                       func, db.m_errno, db.m_tries, db.m_retries, db.m_error.c_str(), sql);
            if (db.isTemporaryError())
                throw CDbTempErrorException(err, db.m_errno);
            else
                throw CDbException(err, db.m_errno);
        }
        if (db.m_tries > 1)
            hlog(HLOG_WARN, "%s: query executed after %u tries; errno was %u",
                 func, db.m_tries, db.m_errno);
    }
    static inline FString DbEscape(CDbConnection &db, const char *sql_in)
    {
        return db.escape(sql_in);
    }
};

#define DbStore(db, sql) CDbUtil::DbStore(__FUNCTION__, db, sql)
#define DbUse(db, sql) CDbUtil::DbUse(__FUNCTION__, db, sql)
#define DbExecute(db, sql) CDbUtil::DbExecute(__FUNCTION__, db, sql)
#define DbEscape(db, sql) CDbUtil::DbEscape(db, sql)
#endif
#endif
