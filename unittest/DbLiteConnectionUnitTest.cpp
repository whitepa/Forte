#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "DbLiteConnection.h"

using namespace Forte;

#define TEST_DB "/tmp/DbLiteConnectionUnitTest.db"
#define TEST_DB_BACKUP "/tmp/DbLiteConnectionUnitTest2.db"

#define CREATE_TEST_TABLE                       \
    "CREATE TABLE `test` ("                     \
    "  `a` int(10) not null default '0',"       \
    "  `b` int(10) not null default '0',"       \
    "  PRIMARY KEY (`a`)"                       \
    ");"

class DbTest: public Forte::DbRow
{
public:
    DbTest() {};
    DbTest(const Forte::DbResultRow &row) { Set(row); }
    void Set(const Forte::DbResultRow &row) {
        mA = GetInt(row, 0);
        mB = GetInt(row, 1);
    }
    int mA;
    int mB;
};

class DbLiteConnectionTest : public ::testing::Test {
public:
    virtual void SetUp() {
        unlink(TEST_DB);
        unlink(TEST_DB_BACKUP);
        mLogManager.BeginLogging("//stderr");
        DbLiteConnection db(SQLITE_OPEN_READWRITE | 
                            SQLITE_OPEN_CREATE);
        db.Init(TEST_DB);
        ASSERT_NO_THROW(db.Execute(CREATE_TEST_TABLE));
    }
    virtual void TearDown() {
        unlink(TEST_DB);
    }
    LogManager mLogManager;
};

TEST_F(DbLiteConnectionTest, QueryExecution)
{
    DbLiteConnection db;
    ASSERT_TRUE(db.Init(TEST_DB));
    ASSERT_NO_THROW(db.Execute(CREATE_TEST_TABLE));
}

TEST_F(DbLiteConnectionTest, Backup)
{
    DbLiteConnection db;
    ASSERT_TRUE(db.Init(TEST_DB));
    ASSERT_NO_THROW(db.Execute(CREATE_TEST_TABLE));
    for (int i = 0; i < 100; i++)
    {
        ASSERT_NO_THROW(db.Execute(FString(FStringFC(),
                                           "INSERT INTO test VALUES (%d, %d)",
                                           i, rand())));
    }
    ASSERT_NO_THROW(db.BackupDatabase(TEST_DB_BACKUP));
    DbLiteConnection dbBackup;
    ASSERT_NO_THROW(dbBackup.Init(TEST_DB_BACKUP));
    DbTest test1, test2;
    {
        DbResult res = db.Store("SELECT sum(a),sum(b) FROM test");
        ASSERT_TRUE(res.FetchRow(test1));
    }
    {
        DbResult res = dbBackup.Store("SELECT sum(a),sum(b) FROM test");
        ASSERT_TRUE(res.FetchRow(test2));
    }
    hlog(HLOG_DEBUG, "1a=%d 1b=%d   2a=%d 2b=%d", 
         test1.mA, test1.mB,
         test2.mA, test2.mB);
    EXPECT_EQ(test1.mA, test2.mA);
    EXPECT_EQ(test1.mB, test2.mB);
    unlink(TEST_DB_BACKUP);
}


// TEST(DbLiteConnection, Locking)
// {
//     DbLiteConnection db;
//     ASSERT_TRUE(db.Init(TEST_DB));

// }

// TEST(DbLiteConnection, LockingWithTimeout)
// {
    
// }

// TEST(DbLiteConnection, SharedConnection)
// {

// }
