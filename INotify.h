#ifndef __FORTE__INOTIFY_H__
#define __FORTE__INOTIFY_H__

#include "AutoFD.h"
#include "AutoMutex.h"
#include "Exception.h"
#include "Object.h"
#include <set>

// [forward declarations]
class inotify_event;

namespace boost
{
    template <class T>
    class shared_ptr;
}

namespace Forte {

    EXCEPTION_CLASS(EINotify);
    EXCEPTION_CLASS(EINotifyAddWatchFailed);
    EXCEPTION_CLASS(EINotifyRemoveWatchFailed);

    /**
    * @class INotify
    *
    * @brief This class encapsulates inotify to simplify it's usage and add useful features.
    *
    *  INotify class cleans used watch descriptors and read file descriptors. The Event subclass
    *  provides a container compatible object encapsulating the non-uniformly sized struct inotify_event.
    *  This class provides an interrupt method which does not use signals but instead subscribes a self
    *  watched file descriptor. The read method has been augmented with a timeout method for those applications
    *  which require a periodic servicing when blocked for a duration of time. Thread safe.
    */
    class INotify : public Forte::Object
    {
    public:
        typedef Forte::Object base_type;

        /**
        * @class Event
        *
        * @brief The Event subclass provides a container compatible object
        * encapsulating the non-uniformly sized struct inotify_event.
        */
        class Event : public Forte::Object
        {
        public:

            /**
            * Constructor
            * @param event inotify_event
            */
            Event(const inotify_event* event);
        private:
            const int wd;
            const uint32_t mask;
            const uint32_t cookie;
            std::string name;

        }; // INotifyEvent

        typedef std::vector<boost::shared_ptr<Event> > EventVector;

        /**
        * Default Ctor
        */
        INotify(const size_t& maxBufferBytes=1024);

        /**
        * Dtor
        */
        virtual ~INotify();

        /**
        * Adds a watch
        * @param path  path of the file to watch
        * @param mask  mask of notifications (e.g. IN_MODIFY)
        * @return  the watched file descriptor
        * @throws  EINotifyAddWatchFailed
        */
        int AddWatch(const std::string& path, const int& mask);

        /**
        * Removes a watch
        * @param fd  watched file descriptor to remove
        * @throws  EINotifyRemoveWatchFailed
        */
        void RemoveWatch(const int& fd);

        /**
        * Reads an array of events from inotify
        * @param secs  seconds to wait before timing out the read operation
        * @return  vector of Events read
        */
        EventVector Read(const time_t& secs);

        /**
        * Interrupts the read operation, causing it to return with current Events
        */
        void Interrupt();

    private:
        std::vector<char> buffer;
        AutoFD mNotifyFd;
        std::set<int> mWatchFds;
        Mutex mMutex;
        AutoFD mKickerFd;
        char mKickerPath[32];
        int mKickerWatchFd;
    }; // INotify

} // forte

#endif // __FORTE__INOTIFY_H__
