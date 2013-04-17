#ifndef FORTE_DB_LITE_CONNECTION_FACTORY
#define FORTE_DB_LITE_CONNECTION_FACTORY

#include <DbConnectionFactory.h>

#ifndef FORTE_NO_DB
#ifdef FORTE_WITH_SQLITE

namespace Forte {

class DbLiteConnectionFactory : public DbConnectionFactory
{
public:

    DbLiteConnectionFactory(const char *vfsName = NULL);
    DbLiteConnectionFactory(int flags, const char *vfsName = NULL);
    DbConnection* create();

private:
    const int mFlags;
    const Forte::FString mVFSName;

}; // class DbLiteConnectionFactory

} // namespace Forte

#endif  // FORTE_WITH_SQLITE
#endif  // FORTE_NO_DB
#endif  // FORTE_DB_LITE_CONNECTION_FACTORY


