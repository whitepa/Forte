#ifndef FORTE_DB_LITE_CONNECTION_FACTORY
#define FORTE_DB_LITE_CONNECTION_FACTORY

#include <DbConnectionFactory.h>

namespace Forte {

class DbLiteConnectionFactory : public DbConnectionFactory
{
public:

    DbLiteConnectionFactory();
    DbLiteConnectionFactory(int flags);
    DbConnection* create();

private:
    const int mFlags;

}; // class DbLiteConnectionFactory

} // namespace Forte

#endif // FORTE_DB_LITE_CONNECTION_FACTORY


