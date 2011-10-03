#ifndef FORTE_DB_CONNECTION_FACTORY
#define FORTE_DB_CONNECTION_FACTORY

#include "Object.h"
#include "DbConnection.h"

namespace Forte {

    class DbConnectionFactory : public Object
    {
    public:
        virtual DbConnection* create()=0;
        virtual ~DbConnectionFactory()
        {
        }
    };

    template <class T>
    class DbConnectionFactoryBase : public DbConnectionFactory
    {
    public:
        DbConnection* create()
        {
            return new T();
        }
    };

} // namespace Forte

#endif // FORTE_DB_CONNECTION_FACTORY




