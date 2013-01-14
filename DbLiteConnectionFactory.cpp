#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_SQLITE

#include "DbLiteConnectionFactory.h"
#include "DbLiteConnection.h"

using namespace Forte;

DbLiteConnectionFactory::DbLiteConnectionFactory(const char *vfsName)
    :mFlags(SQLITE_OPEN_READWRITE),
     mVFSName(vfsName)
{
}

DbLiteConnectionFactory::DbLiteConnectionFactory(int flags, const char *vfsName)
    :mFlags(flags),
     mVFSName(vfsName)
{
}

DbConnection* DbLiteConnectionFactory::create()
{
    return new DbLiteConnection(mFlags, mVFSName);
}


#endif
#endif
