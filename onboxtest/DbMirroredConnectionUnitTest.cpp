#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "LogManager.h"
#include "DbLiteConnection.h"
#include "DbConnectionPool.h"
#include "ProcRunner.h"
#include "AutoDoUndo.h"
#include "FileSystem.h"
#include "DbSqlStatement.h"
#include "DbAutoConnection.h"
#include "DbBackupManagerThread.h"

using namespace Forte;
using namespace boost;

#define CREATE_TEST_TABLE                       \
    "CREATE TABLE `test` ("                     \
    "  `a` int(10) not null default '0',"       \
    "  `b` int(10) not null default '0',"       \
    "  PRIMARY KEY (`a`)"                       \
    ");"

class DatabaseTest : public ::testing::Test
{
protected:
    virtual void CreateDatabases()
    {
    }

    virtual void CreateTables()
    {
    }

    virtual size_t PopulateData()
    {
        return 0;
    }

    virtual void RemoveDatabases()
    {
    }

    virtual const char* getDatabaseName(const char* str="")=0;

    virtual void SetUp()
    {
        mLogManager.BeginLogging("//stderr");

        {
            hlog(HLOG_TRACE, "{");

            RemoveDatabases();
            CreateDatabases();
            CreateTables();

            hlog(HLOG_TRACE, "} ");
        }
    }

    virtual void TearDown()
    {
        hlog(HLOG_TRACE, "{");

        RemoveDatabases();

        hlog(HLOG_TRACE, "}");
    }

private:
    LogManager mLogManager;
};

EXCEPTION_SUBCLASS(Exception, EMountGPFS);
EXCEPTION_SUBCLASS(Exception, EUmountGPFS);


class BasicDatabaseTest : public DatabaseTest
{
protected:
    typedef DatabaseTest base_type;

    virtual void CreateDatabases()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        EXPECT_TRUE(db.Init(getDatabaseName()));
    }

    virtual void CreateTables()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        EXPECT_TRUE(db.Init(getDatabaseName()));
        EXPECT_TRUE(db.Execute(CREATE_TEST_TABLE));
    }

    virtual size_t PopulateData(DbConnection& db)
    {
        const size_t rows(100);
        for (int i = 0; i < rows; i++)
        {
            FString insertSql;
            insertSql.Format("INSERT INTO test VALUES (%d, %d)", i, rand());
            EXPECT_TRUE(db.Execute(InsertDbSqlStatement(insertSql)));
        }

        return rows;
    }

    virtual size_t PopulateData()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE| SQLITE_OPEN_CREATE);
        EXPECT_TRUE(db.Init(getDatabaseName()));

        return PopulateData(db);
    }

    virtual void RemoveDatabases()
    {
        RemovePrimaryDatabase();
        RemoveSecondaryDatabase();
    }

    virtual void RemovePrimaryDatabase()
    {
        unlink(getDatabaseName());
    }

    virtual void RemoveSecondaryDatabase()
    {
        unlink(getBackupDatabaseName());
    }

    virtual void SetUp()
    {
        base_type::SetUp();
    }

    const char* getDatabaseName(const char* str="")
    {
        return "/fsscale0/mirrored_database_test_primary.db";
    }

    const char* getBackupDatabaseName(const char* str="")
    {
        return "/tmp/mirrored_database_test_secondary.db";
    }

public: // drawback, bind requires these be public

    typedef AutoDoUndo<void, void> AutoMountGPFS;

    void unmountDatabase()
    {
        FileSystem fs;

        ASSERT_TRUE(fs.FileExists(getDatabaseName()));

        executeCommand("/etc/init.d/monit stop");
        executeCommand("/etc/init.d/scaled stop");
        executeCommand("/etc/init.d/screpld stop");
        executeCommand("/etc/init.d/gpfs stop");
        sleep(10);

        ASSERT_FALSE(fs.FileExists(getDatabaseName()));
    }

    void mountDatabase()
    {
        FileSystem fs;

        ASSERT_FALSE(fs.FileExists(getDatabaseName()));

        executeCommand("/etc/init.d/gpfs start");
        executeCommand("/etc/init.d/screpld start");
        executeCommand("/etc/init.d/scaled start");
        executeCommand("/etc/init.d/monit start");
        sleep(10);
        executeCommand("mount /fsscale0");
        sleep(5);

        ASSERT_TRUE(fs.FileExists(getDatabaseName()));
    }

    void executeCommand(const FString& cmd)
    {
        int ret;
        FString output;
        ProcRunner procRunner;

        if ((ret = procRunner.Run(cmd, "", &output, PROC_RUNNER_DEFAULT_TIMEOUT)) != 0)
        {
            FString err;
            err.Format("Unable to run '%s' [return code: %i] (%s)", cmd.c_str(), ret, output.c_str());
            hlog(HLOG_ERROR, err);
            throw EUmountGPFS(err);
        }
    }

    boost::function<void()> UnmountDatabaseFunction()
    {
        return boost::bind(&::BasicDatabaseTest::unmountDatabase, this);
    }

    boost::function<void()> MountDatabaseFunction()
    {
        return boost::bind(&::BasicDatabaseTest::mountDatabase, this);
    }
};

TEST_F(BasicDatabaseTest, SqliteBackupDatabaseTest)
{
    const size_t rows = PopulateData();
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName());
    DbConnection& dbConnection(pool.GetDbConnection());

    ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName()));
}

TEST_F(BasicDatabaseTest, SqliteManualFailoverManualBackupDatabaseTest)
{
    const size_t rows = PopulateData();

    {
        DbConnectionPool pool("sqlite_mirrored", getDatabaseName());
        DbConnection& dbConnection(pool.GetDbConnection());

        ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName()));

        pool.ReleaseDbConnection(dbConnection);
    }

    RemovePrimaryDatabase();

    DbConnectionPool pool("sqlite_mirrored", getBackupDatabaseName());
    DbConnection& dbConnection = pool.GetDbConnection();

    DbResult res = dbConnection.Store("SELECT * FROM test");
    EXPECT_EQ(res.GetNumRows(), rows);
}

TEST_F(BasicDatabaseTest, SqliteAutoFailoverAutoBackupDatabaseTest)
{
    size_t rows(0);
    shared_ptr<DbConnectionPool> pool (make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

    {
        DbBackupManagerThread dbBackupMgr(pool);
        DbAutoConnection dbConnection(pool);
        rows = PopulateData(*dbConnection);
    }

    RemovePrimaryDatabase();

    DbAutoConnection dbConnection(pool);
    DbResult res = dbConnection->Store("SELECT * FROM test");

    EXPECT_EQ(res.GetNumRows(), rows);
}


TEST_F(BasicDatabaseTest, SqliteFailedGPFSManualBackupDatabaseTest)
{
    const size_t rows = PopulateData();
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName());

    {
        DbConnection& dbConnection(pool.GetDbConnection());
        ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName()));

        DbResult resBeforeUnmount (dbConnection.Store("SELECT * FROM test"));
        EXPECT_EQ(resBeforeUnmount.GetNumRows(), rows);
        pool.ReleaseDbConnection(dbConnection);
    }

    AutoMountGPFS unmountGPFS(UnmountDatabaseFunction(), MountDatabaseFunction());

    DbConnection& dbConnection(pool.GetDbConnection());

    DbResult resAfterUnmount (dbConnection.Store("SELECT * FROM test"));
    EXPECT_EQ(resAfterUnmount.GetNumRows(), rows);

    ASSERT_NO_THROW(pool.ReleaseDbConnection(dbConnection));
}

TEST_F(BasicDatabaseTest, SqliteFailedGPFSAutoBackupDatabaseTest)
{
    size_t rows(0);
    shared_ptr<DbConnectionPool> pool (make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

    {
        DbBackupManagerThread dbBackupMgr(pool);
        DbAutoConnection dbConnection(pool);
        ASSERT_NO_THROW(rows = PopulateData(*dbConnection));

        DbResult resBeforeUnmount (dbConnection->Store("SELECT * FROM test"));
        EXPECT_EQ(resBeforeUnmount.GetNumRows(), rows);
    }

    AutoMountGPFS unmountGPFS(UnmountDatabaseFunction(), MountDatabaseFunction());

    DbAutoConnection dbConnection(pool);

    DbResult resAfterUnmount (dbConnection->Store("SELECT * FROM test"));
    EXPECT_EQ(resAfterUnmount.GetNumRows(), rows);
}

TEST_F(BasicDatabaseTest, SqliteFailedGPFSPrimaryDbUnderHotDbConnectionDatabaseTest)
{
    size_t rows(0);
    shared_ptr<DbConnectionPool> pool (make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));
    DbResult resAfterUnmount;

    DbAutoConnection dbConnection(pool);
    {
        DbBackupManagerThread dbBackupMgr(pool);
        ASSERT_NO_THROW(rows = PopulateData(*dbConnection));

        DbResult resBeforeUnmount (dbConnection->Store("SELECT * FROM test"));
        EXPECT_EQ(resBeforeUnmount.GetNumRows(), rows);

        AutoMountGPFS unmountGPFS(UnmountDatabaseFunction(), MountDatabaseFunction());

    }

    resAfterUnmount = dbConnection->Store("SELECT * FROM test");
    EXPECT_EQ(resAfterUnmount.GetNumRows(), rows);
}


