#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "DbLiteConnection.h"

using namespace Forte;

LogManager logManager;

#define TEST_DB "/tmp/DbLiteConnectionUnitTest.db"

#define CREATE_TEST_TABLE                       \
    "CREATE TABLE `test` ("                     \
    "  `a` int(10) not null default '0',"       \
    "  `b` int(10) not null default '0',"       \
    "  PRIMARY KEY (`a`)"                       \
    ");"

TEST(DbLiteConnection, CreateDatabase)
{
    DbLiteConnection db(SQLITE_OPEN_READWRITE | 
                        SQLITE_OPEN_CREATE);
    ASSERT_TRUE(db.Init(TEST_DB));
}

TEST(DbLiteConnection, QueryExecution)
{
    DbLiteConnection db;
    ASSERT_TRUE(db.Init(TEST_DB));
    ASSERT_NO_THROW(db.Execute(CREATE_TEST_TABLE));
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
