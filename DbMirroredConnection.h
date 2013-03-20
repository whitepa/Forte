#ifndef FORTE_DB_MIRRORED_CONNECTION
#define FORTE_DB_MIRRORED_CONNECTION

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

class DbMirroredConnection : public DbConnection
{
public:
    typedef DbConnection base_type;

    DbMirroredConnection(boost::shared_ptr<DbConnectionFactory> primaryDbConnectionFactory,
                         boost::shared_ptr<DbConnectionFactory> secondaryDbConnectionFactory,
                         const FString& altDbName);

    ~DbMirroredConnection();

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

    bool Execute(const DbSqlStatement& statement);
    DbResult Use(const DbSqlStatement& statement);
    DbResult Store(const DbSqlStatement& statement);

    virtual const std::string& GetDbName() const;
    virtual const FString& GetCurrentQuery() const;

private:
    bool setActiveSecondary();
    bool isActiveSecondary() const;
    bool isActivePrimary() const;
private:
    boost::shared_ptr<DbConnectionFactory> mPrimaryDbConnectionFactory;
    boost::shared_ptr<DbConnectionFactory> mSecondaryDbConnectionFactory;
    boost::shared_ptr<DbConnection> mDbConnection;
    boost::shared_ptr<DbConnection> mDbConnectionSecondary;
    const FString mAltDbName;
    mutable RecursiveMutex mMutex;

}; // DbMirroredConnection

} // namespace Forte

#endif // FORTE_DB_MIRRORED_CONNECTION


