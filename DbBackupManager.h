#ifndef FORTE_DB_BACKUP_MANAGER
#define FORTE_DB_BACKUP_MANAGER

#include "Object.h"

namespace Forte {

    class DbBackupManager : virtual public Object
    {
    public:
        virtual ~DbBackupManager();
        virtual void* run()=0;
        virtual void Shutdown()=0;

    }; // DbBackupManager

} // namespace Forte

#endif // FORTE_DB_BACKUP_MANAGER

