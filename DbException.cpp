#ifndef FORTE_NO_DB

#include "Forte.h"
using namespace Forte;

DbException::DbException(const char* description, unsigned int dbErrno, const char *sql) :
    Exception((description != NULL) ? description : "Database Exception"),
    mDbErrno(dbErrno),
    mSQL((sql != NULL) ? sql : "")
{
}

DbException::DbException(const DbException& other) :
    Exception(other)
{
    mDbErrno = other.mDbErrno;
    mSQL = other.mSQL;
}

DbException::~DbException() throw()
{
}


DbTempErrorException::DbTempErrorException(const char* description, unsigned int dbErrno,
                                             const char *sql) :
    DbException((description!=NULL)?description:"Temporary Database Exception", dbErrno, sql)
{
}

// DbTempErrorException::DbTempErrorException(const DbTempErrorException& other) :
//   DbException(other)
// {
// }

DbTempErrorException::~DbTempErrorException() throw()
{
}

#endif
