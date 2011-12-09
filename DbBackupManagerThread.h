#include "DbBackupManager.h"
#include "Thread.h"

namespace boost
{
    template <class T>
    class shared_ptr;

    template <class T>
    class scoped_ptr;
}

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

        void addWatchDbParentDir();
        void addWatchDb();
        void resetWatchDbParentDir();
        void resetWatchDb();
        bool hasWatchDb() const;
        bool hasWatchDbParentDir() const;

        enum
        {
            TIMEOUT_MS_BEFORE_CHECKING_MODIFICATION_TIME = 1000
        };

    private:
        boost::shared_ptr<DbConnectionPool> mPool;
        boost::scoped_ptr<INotify> mInotify;
        int mDbWatch;
        int mDbParentDirWatch;
    }; // DbBackupManagerThread

} // namespace Forte
