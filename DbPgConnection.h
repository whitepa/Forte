#ifndef __DbPgConnection_h
#define __DbPgConnection_h

#ifndef FORTE_NO_DB
#ifndef FORTE_NO_POSTGRESQL

#include <libpq-fe.h>
#include "FString.h"
#include "DbConnection.h"
#include "DbException.h"
#include "DbPgResult.h"

class CDbPgConnection : public CDbConnection
{
public:
    CDbPgConnection();
    virtual ~CDbPgConnection();

    // initialization
    virtual bool Init(PGconn *db);

    // connection management
    virtual bool Connect();
    virtual bool Close();

    // queries
    virtual bool execute(const FString& sql);
    virtual CDbResult store(const FString& sql);
    virtual CDbResult use(const FString& sql);

    // error info
    virtual bool isTemporaryError() const;
    
    // misc.
    virtual uint64_t InsertID() { return m_last_res.InsertID(); }
    virtual uint64_t AffectedRows() { return strtoull(PQcmdTuples(m_last_res), NULL, 0); }
    virtual FString escape(const char *str);

private:
    CDbResult query(const FString& sql);
    CDbPgResult m_last_res;
    PGconn *m_db;
};

#endif
#endif
#endif
