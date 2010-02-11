#ifndef DatabaseException_h
#define DatabaseException_h

#ifndef FORTE_NO_DB

#include "Exception.h"
#include "FString.h"

class CDbException : public CException
{
public:
    CDbException(const char* description=NULL, unsigned int dbErrno = 0,
                 const char *sql=NULL);
    CDbException(const CDbException& other);
    virtual ~CDbException() throw();

    unsigned int mDbErrno;
    FString mSQL;
};

class CDbTempErrorException : public CDbException
{
public:
    CDbTempErrorException(const char* description=NULL, unsigned int dbErrno = 0,
                          const char *sql=NULL);
    CDbTempErrorException(const CDbException& other);
    virtual ~CDbTempErrorException() throw();
};

#endif
#endif
