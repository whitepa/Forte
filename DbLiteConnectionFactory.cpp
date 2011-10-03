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



