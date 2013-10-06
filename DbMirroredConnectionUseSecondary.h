#ifndef FORTE_DB_MIRRORED_CONNECTION_USE_SECONDARY
#define FORTE_DB_MIRRORED_CONNECTION_USE_SECONDARY

#include <DbConnection.h>
#include <DbConnectionFactory.h>
#include <DbSqlStatement.h>
#include <Thread.h>

namespace boost
{
    template<class T>
    class shared_ptr;
}

namespace Forte {

class DbMirroredConnectionUseSecondary : public DbConnection
{
public:
    typedef DbConnection base_type;

    DbMirroredConnectionUseSecondary(boost::shared_ptr<DbConnectionFactory> primaryDbConnectionFactory,
                         boost::shared_ptr<DbConnectionFactory> secondaryDbConnectionFactory,
                         const FString& altDbName);

    ~DbMirroredConnectionUseSecondary();

    bool Init(const FString& db,const FString& user,const FString& pass,
              const FString& host = "localhost",const FString& socket = "", unsigned int retries = 20);

    bool Connect();
    bool Close();
    bool HasPendingQueries() const;
    bool Execute(const FString& sql);
    DbResult Store(const FString& sql);
    DbResult Use(const FString& sql);
    bool Execute2(const FString& sql);
    DbResult Store2(const FString& sql);
    DbResult Use2(const FString& sql);
    void AutoCommit(bool enabled);
    void Begin();
    void Commit();
    void Rollback();
    uint64_t InsertID();
    uint64_t AffectedRows();
    FString Escape(const char *str);
    FString GetError() const;
    unsigned int GetErrno() const;
    unsigned int GetTries() const;
    bool IsTemporaryError() const;

    unsigned int GetRetries() const;
    unsigned int GetQueryRetryDelay() const;

    void BackupDatabase(const FString &targetPath);
    void BackupDatabase(DbConnection &targetDatabase);

    bool Execute(const DbSqlStatement& statement);
    DbResult Use(const DbSqlStatement& statement);
    DbResult Store(const DbSqlStatement& statement);

    virtual const std::string& GetDbName() const;
    virtual const FString& GetCurrentQuery() const;

private:
    DbMirroredConnectionUseSecondary() {};
    bool setReadOnlyActive();
    bool setReadWriteActive();
    bool isMutableQuery(const Forte::FString &sql) const
    {
        if (sql.Left(6) == "SELECT" ||
            sql.Left(6) == "select")
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    boost::shared_ptr<DbConnection> selectDbToUse(const DbSqlStatement& statement)
    {
        if (statement.IsMutator())
        {
            hlog(HLOG_DEBUG, "Switching to rw db for statement %s",
                 statement.GetStatement().c_str());
            setReadWriteActive();
            return mDbConnectionReadWrite;
        }

        return selectDbToUse();
    }

    boost::shared_ptr<DbConnection> selectDbToUse(const Forte::FString &sql)
    {
        if (isMutableQuery(sql))
        {
            hlog(HLOG_DEBUG, "Switching to rw db for statement %s",
                 sql.c_str());
            setReadWriteActive();
            return mDbConnectionReadWrite;
        }

        return selectDbToUse();
    }

    boost::shared_ptr<DbConnection> selectDbToUse() const
    {
        if (mReadOnlyDbActive) return mDbConnectionReadOnly;
        else return mDbConnectionReadWrite;
    }

private:
    boost::shared_ptr<DbConnectionFactory> mRWDbConnectionFactory;
    boost::shared_ptr<DbConnectionFactory> mROnlyDbConnectionFactory;
    boost::shared_ptr<DbConnection> mDbConnectionReadOnly;
    boost::shared_ptr<DbConnection> mDbConnectionReadWrite;
    bool mReadOnlyDbActive;
    const FString mAltDbName;
    FString mDbName;
    mutable RecursiveMutex mMutex;

}; // DbMirroredConnectionUseSecondary

} // namespace Forte

#endif // FORTE_DB_MIRRORED_CONNECTION_USE_SECONDARY


