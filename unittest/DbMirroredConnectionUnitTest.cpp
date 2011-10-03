#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/convenience.hpp>
#include "LogManager.h"
#include "DbLiteConnection.h"
#include "DbSqlStatement.h"
#include "DbConnectionPool.h"
#include "DbAutoConnection.h"
#include "DbBackupManager.h"
#include "DbBackupManagerThread.h"

#include "Forte.h"
#include "Context.h"

using namespace Forte;
using boost::shared_ptr;
using boost::make_shared;

#define CREATE_TEST_TABLE                       \
    "CREATE TABLE `test` ("                     \
    "  `a` int(10) not null default '0',"       \
    "  `b` int(10) not null default '0',"       \
    "  PRIMARY KEY (`a`)"                       \
    ");"

#define SELECT_TEST_TABLE                       \
    "SELECT * FROM test"

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
        string databasePath("/tmp/");
        databasePath += boost::filesystem::basename(__FILE__);
        databasePath += ".unittest.db";
        return databasePath.c_str();
    }

    const char* getBackupDatabaseName(const char* str="")
    {
        FString secondaryDatabasePath(getDatabaseName());
        secondaryDatabasePath += ".backup";
        return secondaryDatabasePath.c_str();
    }
};

TEST_F(BasicDatabaseTest, SqliteBackupDatabaseTest)
{
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName());
    DbConnection& dbConnection(pool.GetDbConnection());

    ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName()));
}

TEST_F(BasicDatabaseTest, SqliteSameSourceAndTargetBackupDatabaseTest)
{
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName());
    DbConnection& dbConnection(pool.GetDbConnection());

    ASSERT_THROW(dbConnection.BackupDatabase(getDatabaseName()), Forte::EDbLiteBackupFailed);
}

TEST_F(BasicDatabaseTest, SqliteManualFailoverAndManualBackupDatabaseTest)
{
    size_t rows(0);

    {
        shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite", getDatabaseName()));
        DbAutoConnection dbConnection(pool);

        ASSERT_NO_THROW(dbConnection->BackupDatabase(getBackupDatabaseName()));
    }

    RemovePrimaryDatabase();

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName()));
    DbAutoConnection dbConnection(pool);

    DbResult res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
    EXPECT_EQ(res.GetNumRows(), rows);
}

TEST_F(BasicDatabaseTest, SqliteManualFailoverAutoBackupDatabaseTest)
{
    size_t rows(0);
    DbResult res;

    {
        shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

        {
            DbBackupManagerThread backupMgr(pool);

            {
                DbAutoConnection dbConnection(pool);
                ASSERT_NO_THROW(rows = PopulateData(*dbConnection));
            }
        }

        RemovePrimaryDatabase();

        {
            shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName()));
            DbAutoConnection dbConnection(pool);
            res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
        }
    }

    EXPECT_EQ(res.GetNumRows(), rows);
}


TEST_F(BasicDatabaseTest, ManualBackupAutoFailoverDbMirroredConnectionTest)
{
    PopulateData();

    DbConnectionPool pool("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName());

    {
        DbConnection& dbConnection(pool.GetDbConnection());
        ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName()));

        pool.ReleaseDbConnection(dbConnection);
    }

    RemovePrimaryDatabase();

    DbConnection& dbConnection = pool.GetDbConnection();

    DbResult res = dbConnection.Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
    EXPECT_EQ(res.GetNumRows(), 100);
}

TEST_F(BasicDatabaseTest, AutoBackupAutoFailoverDbMirroredConnectionTest)
{
    size_t rows(0);
    DbResult res;

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

    {
        {
            DbBackupManagerThread backupMgr(pool);

            {
                DbAutoConnection dbConnection(pool);
                ASSERT_NO_THROW(rows = PopulateData(*dbConnection));
            }
        }

        RemovePrimaryDatabase();

        DbAutoConnection dbConnection(pool);
        res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
    }

    EXPECT_EQ(res.GetNumRows(), rows);
}


TEST_F(BasicDatabaseTest, AutoBackupAutoFailoverStabilityOfBackupManagerUnderPrimaryFailureDbMirroredConnectionTest)
{
    size_t rows(0);
    DbResult res;

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));
    DbBackupManagerThread backupMgr(pool);

    {
        {
            DbAutoConnection dbConnection(pool);
            ASSERT_NO_THROW(rows = PopulateData(*dbConnection));
        }

        RemovePrimaryDatabase();

        DbAutoConnection dbConnection(pool);
        res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
    }

    // results here are unknown as the primary could fail before the backup mgr had a chance to finish. test case is ok as long as exceptions aren't thrown
    //EXPECT_EQ(res.GetNumRows(), rows);
}


