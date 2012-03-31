#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/convenience.hpp>
#include "LogManager.h"
#include "FileSystemImpl.h"
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
        mLogManager.BeginLogging(__FILE__ ".log");

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
        for (int i = 0; i < (int)rows; i++)
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

/*
 * Test case where database doesn't exist prior to backup manager instantiation
 */
TEST_F(BasicDatabaseTest, SqliteDbBackupManagerThreadWhenNoInitialDatabaseTest)
{
    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

    RemoveDatabases();

    FileSystemImpl fs;

    ASSERT_FALSE(fs.FileExists(getDatabaseName()));
    ASSERT_FALSE(fs.FileExists(getBackupDatabaseName()));

    {
        DbBackupManagerThread backupMgr(pool);

        CreateDatabases();
        CreateTables();

        // wait for sync
        {
            unsigned int ctr(0);
            while(! fs.FileExists(getBackupDatabaseName()) && (++ctr < 10))
            {
                if(fs.FileExists(getBackupDatabaseName()))
                    break;

                usleep(10000);
            }
        }
    }

    ASSERT_TRUE(fs.FileExists(getDatabaseName()));
    ASSERT_TRUE(fs.FileExists(getBackupDatabaseName()));
}


/*
 * Test case where primary db is future prior to db backup manager instantiation
 */
TEST_F(BasicDatabaseTest, SqliteDbBackupManagerThreadWhenFuturePrimaryDatabaseTest)
{
    unsigned int rows(0);

    // pool to backup database
    shared_ptr<DbConnectionPool> backupPool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName()));

    {
        shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

        // make an empty primary
        RemoveDatabases();
        CreateDatabases();

        // make an initial backup
        {
            DbAutoConnection dbConnection(pool);
            dbConnection->BackupDatabase(getBackupDatabaseName());
        }

        // insert some tables and rows
        CreateTables();
        rows = (PopulateData());

        // create backup
        {
            DbBackupManagerThread backupMgr(pool);

            DbResult res;

            // keep the backup manager thread going long enough to enter the run loop prior to dtor
            unsigned int ctr(0);
            do
            {
                DbAutoConnection dbConnection(backupPool);
                res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
            }
            while(res.GetNumRows() < rows && (++ctr < 10));
        }
    }

    DbAutoConnection dbConnection(backupPool);
    DbResult res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));

    ASSERT_EQ(res.GetNumRows(), rows);
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

TEST_F(BasicDatabaseTest, SqliteNoPathToBackupTargetDatabaseTest)
{
    DbResult res;

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), "/tmp/tmp/tmp/backup.db"));
    DbAutoConnection dbConnection(pool);
    {
        ASSERT_NO_THROW(DbBackupManagerThread backupMgr(pool));
    }
}

TEST_F(BasicDatabaseTest, SqliteNoPrimaryOnAutoBackupDatabaseTest)
{
    DbResult res;

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

    {
        RemovePrimaryDatabase();
        ASSERT_NO_THROW(DbBackupManagerThread backupMgr(pool));
    }
}

TEST_F(BasicDatabaseTest, SqliteManualFailoverAutoBackupDatabaseTest)
{
    size_t rows(0);
    DbResult res;

    {
        shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));
        shared_ptr<DbConnectionPool> backupPool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName()));

        {
            DbBackupManagerThread backupMgr(pool);

            {
                DbAutoConnection dbConnection(pool);
                ASSERT_NO_THROW(rows = PopulateData(*dbConnection));
            }

            DbResult res;

            // wait for sync
            {
                DbResult res;
                unsigned int ctr(0);
                do
                {
                    DbAutoConnection dbConnection(backupPool);
                    res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));

                    if(res.GetNumRows() >= rows)
                        break;

                    usleep(10000);
                }
                while(++ctr < 10);
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
    shared_ptr<DbConnectionPool> backupPool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName()));

    {
        {
            DbBackupManagerThread backupMgr(pool);

            // populate while db backup mgr running
            {
                DbAutoConnection dbConnection(pool);
                ASSERT_NO_THROW(rows = PopulateData(*dbConnection));
            }

            // wait for sync
            {
                DbResult res;
                unsigned int ctr(0);
                do
                {
                    DbAutoConnection dbConnection(backupPool);
                    res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));

                    if(res.GetNumRows() >= rows)
                        break;

                    usleep(10000);
                }
                while(++ctr < 10);
            }
        }

        RemovePrimaryDatabase();

        DbAutoConnection dbConnection(pool);
        res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
    }

    EXPECT_EQ(rows, res.GetNumRows());
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

