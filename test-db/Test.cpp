// test.cpp

#include "Forte.h"
#include "Test.h"

int g_ret = 0;
CDbConnectionPool *g_pool = NULL;

int main(int argc, char *argv[])
{
    char* const fake_args[] = { "test", "-d", "-v" };
    CServerMain foo(3, fake_args, "dv", "");
    foo.mLogManager.BeginLogging("//stderr");

#ifdef FORTE_WITH_MYSQL
    hlog(HLOG_INFO, "MySQL tests");
    setup_mysql();
    run_tests();
#endif

#ifdef FORTE_WITH_POSTGRESQL
    hlog(HLOG_INFO, "PostgreSQL tests");
    setup_pgsql();
    run_tests();
#endif

#ifdef FORTE_WITH_SQLITE
    hlog(HLOG_INFO, "SQLite tests");
    setup_sqlite();
    run_tests();
#endif

    if (g_pool != NULL)
    {
        hlog(HLOG_INFO, "AutoConnection test");
        autoconnection_test();
    }

    hlog(HLOG_INFO, "Testing complete");
    delete g_pool;

    hlog(HLOG_INFO, "%s", (g_ret ? "FAIL" : "PASS"));
    return g_ret;
}


#ifdef FORTE_WITH_MYSQL
void setup_mysql()
{
    CServiceConfig config;

    // create a config
    config.Set("db_type", "mysql");
    config.Set("db_name", "test");
    config.Set("db_user", "qa");
    config.Set("db_pass", "qa");
    config.Set("db_host", "qa");
    config.Set("db_socket", "");
    config.Set("db_poolsize", "1");

    // re-init database connection pool
    delete g_pool;
    g_pool = new CDbConnectionPool(config);
}
#endif


#ifdef FORTE_WITH_POSTGRESQL
void setup_pgsql()
{
    CServiceConfig config;

    // create a config
    config.Set("db_type", "postgresql");
    config.Set("db_name", "qa");
    config.Set("db_user", "qa");
    config.Set("db_pass", "qa");
    config.Set("db_host", "qa");
    config.Set("db_socket", "");
    config.Set("db_poolsize", "1");

    // re-init database connection pool
    delete g_pool;
    g_pool = new CDbConnectionPool(config);
}
#endif


#ifdef FORTE_WITH_SQLITE
void setup_sqlite()
{
    CServiceConfig config;

    // create a config
    config.Set("db_type", "sqlite");
    config.Set("db_name", ":memory:");
    config.Set("db_user", "");
    config.Set("db_pass", "");
    config.Set("db_host", "");
    config.Set("db_socket", "");
    config.Set("db_poolsize", "1");

    // re-init database connection pool
    delete g_pool;
    g_pool = new CDbConnectionPool(config);
}
#endif


void log_row(const CDbResultRow& row)
{
    FString tmp;
    size_t i, n = row.size();
    vector<FString> frow;

    for (i=0; i<n; i++)
    {
        frow.push_back(row[i] ? row[i] : "NULL");
    }

    tmp = FString::join(frow, "\t");
    hlog(HLOG_DEBUG, "ROW: %s", tmp.c_str());
}


void run_tests()
{
    FString sql;
    CDbConnection *ec = NULL;

    try
    {
        // connect
        hlog(HLOG_INFO, "Open test");
        CDbConnection& conn = g_pool->GetDbConnection();
        ec = &conn;
        CDbResult res;
        CDbResultRow row;

        // create a simple table
        hlog(HLOG_INFO, "Creation test");
        sql = "create temporary table foo ( bar int )";
        if (!conn.execute(sql)) throw "execute() failed";

        // insert some data
        hlog(HLOG_INFO, "Insert test");
        sql = "insert into foo(bar) values (1)";
        if (!conn.execute(sql)) throw "execute() failed";

        sql = "insert into foo(bar) values (2)";
        if (!conn.execute(sql)) throw "execute() failed";

        sql = "insert into foo(bar) values (3)";
        if (!conn.execute(sql)) throw "execute() failed";

        sql = "insert into foo(bar) values (NULL)";
        if (!conn.execute(sql)) throw "execute() failed";

        // select some data
        hlog(HLOG_INFO, "Select test");
        sql = "select * from foo";
        if (!(res = conn.store(sql))) throw "store() failed";

        while (res.fetchRow(row))
        {
            log_row(row);
        }

        // drop table
        hlog(HLOG_INFO, "Drop test");
        sql = "drop table foo";
        if (!conn.execute(sql)) throw "execute() failed";

        // error test
        hlog(HLOG_INFO, "Syntax error test");
        sql = "this is a syntax error for sure, dude.";
        if (conn.execute(sql)) throw "syntax error test failed!";

        // error test
        hlog(HLOG_INFO, "SQL error test");
        sql = "select * from table_which_doesnt_exist_dude";
        if (conn.execute(sql)) throw "sql error test failed!";

        // release
        hlog(HLOG_INFO, "Close test");
        g_pool->ReleaseDbConnection(conn);
    }
    catch (CDbException exp)
    {
        hlog(HLOG_DEBUG, "CDbException: [%u] %s", exp.mDbErrno, exp.what());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (CException exp)
    {
        hlog(HLOG_DEBUG, "CException: %s", exp.what());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (FString exp)
    {
        hlog(HLOG_DEBUG, "FString: %s", exp.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (const char *err)
    {
        hlog(HLOG_DEBUG, "Error: %s", err);
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (std::exception exp)
    {
        hlog(HLOG_DEBUG, "std::exception: %s", exp.what());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (...)
    {
        hlog(HLOG_DEBUG, "Unknown exception");
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
}


void autoconnection_test()
{
    FString sql;
    CDbConnection *ec = NULL;

    try
    {
        hlog(HLOG_INFO, "open test");
        CDbAutoConnection db(*g_pool);
        CDbConnection& conn = db;
        ec = db;
        CDbResult res;
        CDbResultRow row;

        hlog(HLOG_INFO, "select 1 test");
        sql = "SELECT 1";
        if (!(res = conn.store(sql))) throw "store() failed";

        while (res.fetchRow(row))
        {
            log_row(row);
        }

        hlog(HLOG_INFO, "close test");
    }
    catch (CDbException exp)
    {
        hlog(HLOG_DEBUG, "CDbException: [%u] %s", exp.mDbErrno, exp.what());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (CException exp)
    {
        hlog(HLOG_DEBUG, "CException: %s", exp.what());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (FString exp)
    {
        hlog(HLOG_DEBUG, "FString: %s", exp.c_str());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (const char *err)
    {
        hlog(HLOG_DEBUG, "Error: %s", err);
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (std::exception exp)
    {
        hlog(HLOG_DEBUG, "std::exception: %s", exp.what());
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
    catch (...)
    {
        hlog(HLOG_DEBUG, "Unknown exception");
        hlog(HLOG_DEBUG, "SQL: %s", sql.c_str());
        hlog(HLOG_DEBUG, "Error: %s", ec ? ec->m_error.c_str() : "[no connection]");
        g_ret++;
    }
}
