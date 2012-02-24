#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_SQLITE

#include "DbLiteConnectionFactory.h"
#include "DbLiteConnection.h"

using namespace Forte;

DbLiteConnectionFactory::DbLiteConnectionFactory()
    :mFlags(SQLITE_OPEN_READWRITE)
{
}

DbLiteConnectionFactory::DbLiteConnectionFactory(int flags)
    :mFlags(flags)
{
}

DbConnection* DbLiteConnectionFactory::create()
{
    return new DbLiteConnection(mFlags);
}


#endif
#endif
