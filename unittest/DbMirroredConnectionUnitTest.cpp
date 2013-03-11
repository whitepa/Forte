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

    virtual std::string getDatabaseName(const char* str="")=0;

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

protected:
    FileSystemImpl mFileSystem;

private:
    LogManager mLogManager;
};


class DbMirroredConnectionUnitTest : public DatabaseTest
{
protected:
    typedef DatabaseTest base_type;

    virtual void CreateDatabases()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        EXPECT_TRUE(db.Init(getDatabaseName().c_str()));
    }

    virtual void CreateTables()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        EXPECT_TRUE(db.Init(getDatabaseName().c_str()));
        EXPECT_TRUE(db.Execute(CREATE_TEST_TABLE));
    }

    virtual size_t PopulateData(DbConnection& db)
    {
        const size_t rows(100);
        for (int i = 0; i < static_cast<int>(rows); i++)
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
        EXPECT_TRUE(db.Init(getDatabaseName().c_str()));

        return PopulateData(db);
    }

    virtual void RemoveDatabases()
    {
        RemovePrimaryDatabase();
        RemoveSecondaryDatabase();
    }

    virtual void RemovePrimaryDatabase()
    {
        unlink(getDatabaseName().c_str());
    }

    virtual void RemoveSecondaryDatabase()
    {
        unlink(getBackupDatabaseName().c_str());
    }

    virtual void SetUp()
    {
        base_type::SetUp();
    }

    string getDatabaseName(const char* str="")
    {
        string databasePath("/tmp/");
        databasePath += boost::filesystem::basename(__FILE__);
        databasePath += getpid();
        databasePath += ".unittest.db";
        return databasePath;
    }

    string getBackupDatabaseName(const char* str="")
    {
        FString secondaryDatabasePath(getDatabaseName());
        secondaryDatabasePath += ".backup";
        return secondaryDatabasePath;
    }
};

TEST_F(DbMirroredConnectionUnitTest, SqliteBackupDatabaseTest)
{
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName().c_str());
    DbConnection& dbConnection(pool.GetDbConnection());


    ASSERT_NO_THROW(DbExecute(dbConnection, "Insert INTO test VALUES (0, 0)"));
    ASSERT_NO_THROW(DbExecute(dbConnection, "Insert INTO test VALUES (1, 1)"));
    try
    {
        DbExecute(dbConnection, "INSERT INTO test VALUES (1, 2)");
        FAIL();
    }
    catch (DbException &e)
    {
        hlog(HLOG_INFO, "%s (%d)", e.what(), e.mDbErrno);
        ASSERT_EQ(SQLITE_CONSTRAINT, e.mDbErrno);
    }
}

TEST_F(DbMirroredConnectionUnitTest, SqliteTestConstraintsError)
{
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName().c_str());
    DbConnection& dbConnection(pool.GetDbConnection());

    ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName().c_str()));
    pool.ReleaseDbConnection(dbConnection);
    pool.DeleteConnections();
}

/*
 * Test case where database doesn't exist prior to backup manager instantiation
 */
TEST_F(DbMirroredConnectionUnitTest, SqliteDbBackupManagerThreadWhenNoInitialDatabaseTest)
{
    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName().c_str(), getBackupDatabaseName().c_str()));

    RemoveDatabases();

    FileSystemImpl fs;

    ASSERT_FALSE(fs.FileExists(getDatabaseName().c_str()));
    ASSERT_FALSE(fs.FileExists(getBackupDatabaseName().c_str()));

    {
        DbBackupManagerThread backupMgr(pool);

        CreateDatabases();
        CreateTables();

        // wait for sync
        {
            unsigned int ctr(0);
            while(! fs.FileExists(getBackupDatabaseName().c_str()) && (++ctr < 10))
            {
                if(fs.FileExists(getBackupDatabaseName().c_str()))
                    break;

                usleep(10000);
            }
        }
    }

    ASSERT_TRUE(fs.FileExists(getDatabaseName().c_str()));
    ASSERT_TRUE(fs.FileExists(getBackupDatabaseName().c_str()));
}


/*
 * Test case where primary db is future prior to db backup manager instantiation
 */
TEST_F(DbMirroredConnectionUnitTest, SqliteDbBackupManagerThreadWhenFuturePrimaryDatabaseTest)
{
    unsigned int rows(0);

    // pool to backup database
    shared_ptr<DbConnectionPool> backupPool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName().c_str()));

    {
        shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName().c_str(), getBackupDatabaseName().c_str()));

        // make an empty primary
        RemoveDatabases();
        CreateDatabases();

        // make an initial backup
        {
            DbAutoConnection dbConnection(pool);
            dbConnection->BackupDatabase(getBackupDatabaseName().c_str());
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

TEST_F(DbMirroredConnectionUnitTest, SqliteSameSourceAndTargetBackupDatabaseTest)
{
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName().c_str());
    DbConnection& dbConnection(pool.GetDbConnection());

    ASSERT_THROW(dbConnection.BackupDatabase(getDatabaseName().c_str()),
                 Forte::EDbLiteBackupFailed);
    pool.ReleaseDbConnection(dbConnection);
    pool.DeleteConnections();
}

TEST_F(DbMirroredConnectionUnitTest, SqliteManualFailoverAndManualBackupDatabaseTest)
{
    size_t rows(0);

    {
        shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite", getDatabaseName().c_str()));
        DbAutoConnection dbConnection(pool);

        ASSERT_NO_THROW(dbConnection->BackupDatabase(getBackupDatabaseName().c_str()));
    }

    RemovePrimaryDatabase();

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName().c_str()));
    DbAutoConnection dbConnection(pool);

    DbResult res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
    EXPECT_EQ(res.GetNumRows(), rows);
}

TEST_F(DbMirroredConnectionUnitTest, SqliteNoPathToBackupTargetDatabaseTest)
{
    DbResult res;
    FString tmpDeepDir(FStringFC(), "/tmp/%d/%d/backup.db", getpid(), getpid());
    FString cleanupPath(FStringFC(), "/tmp/%d", getpid());
    shared_ptr<DbConnectionPool> pool(
        make_shared<DbConnectionPool>(
            "sqlite_mirrored", getDatabaseName().c_str(),
            tmpDeepDir));
    DbAutoConnection dbConnection(pool);
    {
        ASSERT_NO_THROW(DbBackupManagerThread backupMgr(pool));
    }

    const bool unlinkChildren = true;
    mFileSystem.Unlink(cleanupPath, unlinkChildren);
}

TEST_F(DbMirroredConnectionUnitTest, SqliteNoPrimaryOnAutoBackupDatabaseTest)
{
    DbResult res;

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName().c_str(), getBackupDatabaseName().c_str()));

    {
        RemovePrimaryDatabase();
        ASSERT_NO_THROW(DbBackupManagerThread backupMgr(pool));
    }
}

TEST_F(DbMirroredConnectionUnitTest, SqliteManualFailoverAutoBackupDatabaseTest)
{
    size_t rows(0);
    DbResult res;

    {
        shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName().c_str(), getBackupDatabaseName().c_str()));
        shared_ptr<DbConnectionPool> backupPool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName().c_str()));

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
            shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName().c_str()));
            DbAutoConnection dbConnection(pool);
            res = dbConnection->Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
        }
    }

    EXPECT_EQ(res.GetNumRows(), rows);
}


TEST_F(DbMirroredConnectionUnitTest, ManualBackupAutoFailoverDbMirroredConnectionTest)
{
    PopulateData();

    DbConnectionPool pool("sqlite_mirrored", getDatabaseName().c_str(), getBackupDatabaseName().c_str());

    {
        DbConnection& dbConnection(pool.GetDbConnection());
        ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName().c_str()));

        pool.ReleaseDbConnection(dbConnection);
    }

    RemovePrimaryDatabase();

    DbConnection& dbConnection = pool.GetDbConnection();

    DbResult res = dbConnection.Store(SelectDbSqlStatement(SELECT_TEST_TABLE));
    EXPECT_EQ(res.GetNumRows(), 100);
    pool.ReleaseDbConnection(dbConnection);
    pool.DeleteConnections();
}

TEST_F(DbMirroredConnectionUnitTest, AutoBackupAutoFailoverDbMirroredConnectionTest)
{
    size_t rows(0);
    DbResult res;

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName().c_str(), getBackupDatabaseName().c_str()));
    shared_ptr<DbConnectionPool> backupPool(make_shared<DbConnectionPool>("sqlite", getBackupDatabaseName().c_str()));

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


TEST_F(DbMirroredConnectionUnitTest, AutoBackupAutoFailoverStabilityOfBackupManagerUnderPrimaryFailureDbMirroredConnectionTest)
{
    size_t rows(0);
    DbResult res;

    shared_ptr<DbConnectionPool> pool(make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName().c_str(), getBackupDatabaseName().c_str()));
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

