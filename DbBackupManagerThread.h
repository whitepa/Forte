#include "DbBackupManager.h"
#include "Thread.h"
#include <boost/shared_ptr.hpp>

namespace Forte {

    class DbConnectionPool;
    class INotify;

    class DbBackupManagerThread : public DbBackupManager, public Thread
    {
    public:
        typedef DbBackupManager base_type;

        DbBackupManagerThread(boost::shared_ptr<DbConnectionPool> pool);
        ~DbBackupManagerThread();
        void* run();
        void Shutdown();

    protected:
        timespec getModificationTime() const;
        void backupDb();

        enum
        {
            TIMEOUT_SECS_BEFORE_CHECKING_MODIFICATION_TIME = 1
        };

    private:
        boost::shared_ptr<DbConnectionPool> mPool;
        boost::shared_ptr<INotify> mInotify;

    }; // DbBackupManagerThread

} // namespace Forte
