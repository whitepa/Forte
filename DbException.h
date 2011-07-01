#ifndef DatabaseException_h
#define DatabaseException_h

#ifndef FORTE_NO_DB

#include "Exception.h"
#include "FString.h"
namespace Forte
{
    class DbException : public Exception
    {
    public:
        DbException(const char* description=NULL, unsigned int dbErrno = 0,
                     const char *sql=NULL);
        DbException(const DbException& other);
        virtual ~DbException() throw();

        unsigned int mDbErrno;
        FString mSQL;
    };

    class DbTempErrorException : public DbException
    {
    public:
        DbTempErrorException(const char* description=NULL, unsigned int dbErrno = 0,
                              const char *sql=NULL);
        DbTempErrorException(const DbException& other);
        virtual ~DbTempErrorException() throw();
    };

    class DbLookupFailedException : public DbException
    {
    public:
        DbLookupFailedException(const char* description=NULL, unsigned int dbErrno = 0,
                              const char *sql=NULL);
        DbLookupFailedException(const DbException& other);
        virtual ~DbLookupFailedException() throw();
    };
};
#endif
#endif
