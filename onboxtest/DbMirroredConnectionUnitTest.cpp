#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "LogManager.h"
#include "DbLiteConnection.h"
#include "DbConnectionPool.h"
#include "ProcRunner.h"
#include "AutoDoUndo.h"
#include "FileSystemImpl.h"
#include "DbSqlStatement.h"
#include "DbAutoConnection.h"
#include "DbBackupManagerThread.h"
#include "FTrace.h"

// #SCQAD TEST: ONBOX: DBMirroredConnectionUnitTest

// #SCQAD TESTTAG: forte, scel.sqlite

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


class DbMirroredConnectionTest : public DatabaseTest
{
protected:
    typedef DatabaseTest base_type;

    virtual void CreateDatabases()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        ASSERT_TRUE(db.Init(getDatabaseName()));
    }

    virtual void CreateTables()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        ASSERT_TRUE(db.Init(getDatabaseName()));
        ASSERT_TRUE(db.Execute(CREATE_TEST_TABLE));
    }

    virtual size_t PopulateData(DbConnection& db)
    {
        const size_t rows(10);
        for (int i = 0; i < (int)rows; i++)
        {
            FString insertSql;
            insertSql.Format("INSERT INTO test VALUES (%d, %d)", i, rand());
            db.Execute(InsertDbSqlStatement(insertSql));
        }

        return rows;
    }

    virtual size_t PopulateData()
    {
        DbLiteConnection db (SQLITE_OPEN_READWRITE| SQLITE_OPEN_CREATE);
        db.Init(getDatabaseName());

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
        restoreMounts();
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

    void unmountDatabase()
    {
        FileSystemImpl fs;

        ASSERT_TRUE(fs.FileExists(getDatabaseName())) << getDatabaseName();

        executeCommand("/opt/scale/bin/scdoall touch /tmp/cluster-in-shutdown-period");
        executeCommand("/etc/init.d/monit stop");
        executeCommand("/etc/init.d/scaled stop");
        executeCommand("/etc/init.d/screpld stop");
        executeCommand("/etc/init.d/virtlockd stop");
        executeCommand("/usr/lpp/mmfs/bin/mmshutdown");

        waitMountsDown();

        ASSERT_FALSE(fs.FileExists(getDatabaseName())) << getDatabaseName();
    }

    void waitMountsDown()
    {
        for(unsigned long i = 0 ; i < 12 ; i++)
        {
            if(! checkMounts())
            {
                break;
            }
            sleep(10);
        }
    }

    bool checkMounts()
    {
        try
        {
            return ((executeCommand("/bin/mountpoint -q /fsscale0") == 0) &&
                    (executeCommand("/bin/mountpoint -q /fs0") == 0));
        }
        catch(Forte::Exception& e)
        {
            return false;
        }
    }

    void restoreMounts()
    {
        for(unsigned long i = 0 ; i < 60 ; i++)
        {
            if(! checkMounts())
            {
                try
                {
                    executeCommand("/usr/lpp/mmfs/bin/mmmount all -a");
                }
                catch(...)
                {
                }
            }
            else
            {
                break;
            }
            sleep(1);
        }
    }

    void mountDatabase()
    {
        FTRACE;

        FileSystemImpl fs;

        ASSERT_FALSE(fs.FileExists(getDatabaseName())) << getDatabaseName();

        executeCommand("/usr/lpp/mmfs/bin/mmstartup");
        executeCommand("/etc/init.d/virtlockd start");
        executeCommand("/etc/init.d/screpld start");
        executeCommand("/etc/init.d/scaled start");
        executeCommand("/etc/init.d/monit start");
        restoreMounts();
        executeCommand("/opt/scale/bin/scdoall rm /tmp/cluster-in-shutdown-period");

        ASSERT_TRUE(fs.FileExists(getDatabaseName())) << getDatabaseName();
    }

    int executeCommand(const FString& cmd)
    {
        int ret;
        FString output;
        ProcRunner procRunner;

        if ((ret = procRunner.Run(cmd, "", &output, PROC_RUNNER_DEFAULT_TIMEOUT)) != 0)
        {
            FString err;
            err.Format("Unable to run '%s' [return code: %i] (%s)", cmd.c_str(),
                    ret, output.c_str());
            hlogstream(HLOG_ERROR, err);
            throw EUmountGPFS(err);
        }
        else if (!output.empty())
        {
            hlog(HLOG_INFO, "Command %s succeeded with output (%s)",
                 cmd.c_str(),
                 output.c_str());
        }
        else
        {
            hlog(HLOG_INFO, "Command %s succeeded with no output",
                 cmd.c_str());
        }

        return ret;
    }

    boost::function<void()> UnmountDatabaseFunction()
    {
        return boost::bind(&::DbMirroredConnectionTest::unmountDatabase, this);
    }

    boost::function<void()> MountDatabaseFunction()
    {
        return boost::bind(&::DbMirroredConnectionTest::mountDatabase, this);
    }
};

TEST_F(DbMirroredConnectionTest, SqliteBackupDatabaseTest)
{
    FTRACE;

    const size_t __attribute__((unused)) rows = PopulateData();
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName());
    DbConnection& dbConnection(pool.GetDbConnection());

    ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName()));
}

TEST_F(DbMirroredConnectionTest, SqliteManualFailoverManualBackupDatabaseTest)
{
    FTRACE;

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
    ASSERT_EQ(res.GetNumRows(), rows);
}

TEST_F(DbMirroredConnectionTest, SqliteAutoFailoverAutoBackupDatabaseTest)
{
    FTRACE;

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

    ASSERT_EQ(res.GetNumRows(), rows);
}


TEST_F(DbMirroredConnectionTest, SqliteFailedGPFSManualBackupDatabaseTest)
{
    FTRACE;

    const size_t rows = PopulateData();
    DbConnectionPool pool("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName());

    {
        DbConnection& dbConnection(pool.GetDbConnection());
        ASSERT_NO_THROW(dbConnection.BackupDatabase(getBackupDatabaseName()));

        DbResult resBeforeUnmount (dbConnection.Store("SELECT * FROM test"));
        ASSERT_EQ(resBeforeUnmount.GetNumRows(), rows);
        pool.ReleaseDbConnection(dbConnection);
    }

    AutoDoUndo unmountGPFS(UnmountDatabaseFunction(), MountDatabaseFunction());

    DbConnection& dbConnection(pool.GetDbConnection());

    DbResult resAfterUnmount (dbConnection.Store("SELECT * FROM test"));
    ASSERT_EQ(resAfterUnmount.GetNumRows(), rows);

    ASSERT_NO_THROW(pool.ReleaseDbConnection(dbConnection));
}

TEST_F(DbMirroredConnectionTest, SqliteFailedGPFSAutoBackupDatabaseTest)
{
    FTRACE;

    size_t rows(0);
    shared_ptr<DbConnectionPool> pool (make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));

    {
        DbBackupManagerThread dbBackupMgr(pool);
        DbAutoConnection dbConnection(pool);
        ASSERT_NO_THROW(rows = PopulateData(*dbConnection));

        DbResult resBeforeUnmount (dbConnection->Store("SELECT * FROM test"));
        ASSERT_EQ(resBeforeUnmount.GetNumRows(), rows);
    }

    AutoDoUndo unmountGPFS(UnmountDatabaseFunction(), MountDatabaseFunction());

    DbAutoConnection dbConnection(pool);

    DbResult resAfterUnmount (dbConnection->Store("SELECT * FROM test"));
    ASSERT_EQ(resAfterUnmount.GetNumRows(), rows);
}

TEST_F(DbMirroredConnectionTest, SqliteFailedGPFSPrimaryDbUnderHotDbConnectionDatabaseTest)
{
    FTRACE;

    size_t rows(0);
    shared_ptr<DbConnectionPool> pool (make_shared<DbConnectionPool>("sqlite_mirrored", getDatabaseName(), getBackupDatabaseName()));
    DbResult resAfterUnmount;

    DbAutoConnection dbConnection(pool);
    {
        DbBackupManagerThread dbBackupMgr(pool);
        ASSERT_NO_THROW(rows = PopulateData(*dbConnection));

        DbResult resBeforeUnmount (dbConnection->Store("SELECT * FROM test"));
        ASSERT_EQ(resBeforeUnmount.GetNumRows(), rows);

        AutoDoUndo unmountGPFS(UnmountDatabaseFunction(), MountDatabaseFunction());

    }

    resAfterUnmount = dbConnection->Store("SELECT * FROM test");
    ASSERT_EQ(resAfterUnmount.GetNumRows(), rows);
}
