#ifndef __DB_Transaction__h_
#define __DB_Transaction__h_

#ifndef FORTE_NO_DB

#include "DbConnection.h"
#include "LogManager.h"

// define row locking modes for selects
#define REV_DB_NO_LOCK     0
#define REV_DB_LOCK_UPDATE 1
#define REV_DB_LOCK_SHARED 2

namespace Forte
{
    class DbAutoTrans {
    public:
        DbAutoTrans(DbConnection &db_in) :
            db(db_in)
            {
                if (db.HasPendingQueries())
                {
                    hlog(HLOG_ERR, "DbAutoTrans(): new transaction created while queries pending");
                    db.Commit();
                }
                db.AutoCommit(false);
            };
        virtual ~DbAutoTrans() {
            try {
                if (db.HasPendingQueries())
                {
                    hlog(HLOG_DEBUG, "~DbAutoTrans(): Rolling back transaction started at %s:%u",
                         file, line);
                    db.Rollback();
                }
            } catch (...) {
                hlog(HLOG_ERR, "DbAutoTrans(): exception during rollback");
            }
            db.AutoCommit(true);
        }
        
    protected:
        DbConnection &db;

    public:
        const char *file;
        unsigned int line;
    };
};

#define DbStartTrans(db)                        \
    int _dbTransTries = 5;                      \
    bool _dbTransCompleted = false;             \
    do                                          \
    {                                           \
        DbAutoTrans _dbAutoTrans(db);          \
        _dbAutoTrans.file = __FILE__;           \
        _dbAutoTrans.line = __LINE__;           \
        try                                     \
        {
            
#define DbEndTrans()                                    \
    _dbTransCompleted = true;                           \
    }                                                   \
        catch (DbTempErrorException &e)                \
        {                                               \
            if (_dbTransTries == 0)                     \
                throw e;                                \
        }                                               \
    } while (!_dbTransCompleted && _dbTransTries--);

#endif
#endif
