#ifndef __DbAutoConnection_h
#define __DbAutoConnection_h

#ifndef FORTE_NO_DB

#include "Context.h"
#include "DbConnection.h"
#include "DbConnectionPool.h"
#include "LogManager.h"
#include "Object.h"

namespace Forte
{
    class DbAutoConnection : public Object {
    public:
        DbAutoConnection(const Forte::Context &ctxt,
                         const char *poolname,
                         bool autocommit = true)
            : mPoolPtr(ctxt.Get<DbConnectionPool>(poolname)), 
              mDbConnection(mPoolPtr->GetDbConnection())
            {
                // set autocommit appropriately
                AutoCommit(autocommit);
            }
        DbAutoConnection(shared_ptr<DbConnectionPool> poolPtr,
                         bool autocommit = true)
            : mPoolPtr(poolPtr), 
              mDbConnection(mPoolPtr->GetDbConnection())
            {
                // set autocommit appropriately
                AutoCommit(autocommit);
            }

    private:
        // disallow the copy constructor, we don't refcount so the
        // destructor would release the DB connection from a copy before
        // the original may be finished.  Plus, this would potentially
        // allow sharing of a db connection.
        DbAutoConnection(const DbAutoConnection &other) : 
            mPoolPtr(other.mPoolPtr), mDbConnection(other.mDbConnection) { 
            // should never execute
            throw Exception();
        }

    public:
        virtual ~DbAutoConnection() {
            // turn off temporary logging
            LogQueries(false);
            try {
                if (mDbConnection.HasPendingQueries())
                {
                    hlog(HLOG_WARN, "rolling back pending queries on connection");
                    Rollback();
                }
                mPoolPtr->ReleaseDbConnection(mDbConnection);
            } catch (...) {
            }
        }

        inline void AutoCommit(bool autocommit) {
            mDbConnection.AutoCommit(autocommit);
        };
        inline void Begin(void) {
            mDbConnection.Begin();
        };
        inline void Commit(void) {
            mDbConnection.Commit();
        };
        inline void Rollback(void) {
            mDbConnection.Rollback();
        };
        inline void LogQueries(bool log) {
            mDbConnection.LogQueries(log);
        }
    
        // Cast operator
        inline operator DbConnection* () const {
            return &mDbConnection;
        };
        inline operator DbConnection& () const {
            return mDbConnection;
        };
    
        // Dereference operator
        inline DbConnection& operator*() const {
            return mDbConnection;
        };

        // Dereference operator
        inline DbConnection* operator->() const {
            return &mDbConnection;
        };
    
    private:
        shared_ptr<DbConnectionPool> mPoolPtr;
        DbConnection & mDbConnection;
    };
};
#endif
#endif
