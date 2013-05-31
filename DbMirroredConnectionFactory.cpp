#include <DbMirroredConnectionFactory.h>
#include <DbMirroredConnection.h>
#include <boost/shared_ptr.hpp>
#include "FTrace.h"

using namespace Forte;
using namespace boost;

DbMirroredConnectionFactory::DbMirroredConnectionFactory(boost::shared_ptr<DbConnectionFactory> primaryDbConnectionFactory,
                                                         boost::shared_ptr<DbConnectionFactory> secondaryDbConnectionFactory,
                                                         const FString& altDbName)
    :mPrimaryDbConnectionFactory(primaryDbConnectionFactory),
     mSecondaryDbConnectionFactory(secondaryDbConnectionFactory),
     mAltDbName(altDbName)
{
}

DbConnection* DbMirroredConnectionFactory::DbMirroredConnectionFactory::create()
{
    FTRACE;
    return new DbMirroredConnection(mPrimaryDbConnectionFactory, mSecondaryDbConnectionFactory, mAltDbName);
}
