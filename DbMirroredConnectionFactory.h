#ifndef FORTE_DB_MIRRORED_CONNECTION_FACTORY
#define FORTE_DB_MIRRORED_CONNECTION_FACTORY

#include <DbConnectionFactory.h>

namespace boost
{
    template<class T>
    class shared_ptr;
}

namespace Forte {

class DbMirroredConnectionFactory : public DbConnectionFactory
{
public:
    DbMirroredConnectionFactory(boost::shared_ptr<DbConnectionFactory> primaryDbConnectionFactory,
                                boost::shared_ptr<DbConnectionFactory> secondaryDbConnectionFactory,
                                const FString& altDbName);
    DbConnection* create();

private:
    boost::shared_ptr<DbConnectionFactory> mPrimaryDbConnectionFactory;
    boost::shared_ptr<DbConnectionFactory> mSecondaryDbConnectionFactory;
    const FString mAltDbName;

}; // class DbMirroredConnectionFactory

} // namespace Forte

#endif // FORTE_DB_MIRRORED_CONNECTION_FACTORY


