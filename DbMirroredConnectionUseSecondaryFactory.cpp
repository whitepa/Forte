#include <DbMirroredConnectionUseSecondaryFactory.h>
#include <DbMirroredConnectionUseSecondary.h>
#include <boost/shared_ptr.hpp>
#include "FTrace.h"

using namespace Forte;
using namespace boost;

DbMirroredConnectionUseSecondaryFactory::DbMirroredConnectionUseSecondaryFactory(
    boost::shared_ptr<DbConnectionFactory> primaryDbConnectionFactory,
    boost::shared_ptr<DbConnectionFactory> secondaryDbConnectionFactory,
    const FString& altDbName)
    :mPrimaryDbConnectionFactory(primaryDbConnectionFactory),
     mSecondaryDbConnectionFactory(secondaryDbConnectionFactory),
     mAltDbName(altDbName)
{
}

DbConnection* DbMirroredConnectionUseSecondaryFactory::DbMirroredConnectionUseSecondaryFactory::create()
{
    FTRACE;
    return new DbMirroredConnectionUseSecondary(mPrimaryDbConnectionFactory, mSecondaryDbConnectionFactory, mAltDbName);
}
