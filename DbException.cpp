#ifndef FORTE_NO_DB

#include "DbException.h"

CDbException::CDbException(const char* description, unsigned int dbErrno, const char *sql) :
    CException((description != NULL) ? description : "Database Exception"),
    mDbErrno(dbErrno),
    mSQL((sql != NULL) ? sql : "")
{
}

CDbException::CDbException(const CDbException& other) :
    CException(other)
{
    mDbErrno = other.mDbErrno;
    mSQL = other.mSQL;
}

CDbException::~CDbException() throw()
{
}


CDbTempErrorException::CDbTempErrorException(const char* description, unsigned int dbErrno,
                                             const char *sql) :
    CDbException((description!=NULL)?description:"Temporary Database Exception", dbErrno, sql)
{
}

// CDbTempErrorException::CDbTempErrorException(const CDbTempErrorException& other) :
//   CDbException(other)
// {
// }

CDbTempErrorException::~CDbTempErrorException() throw()
{
}

#endif
