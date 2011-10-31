#include "INotify.h"
#include "FileSystem.h"
#include <sys/inotify.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace Forte;
using namespace boost;
using namespace std;

INotify::Event::Event(const inotify_event* event): wd(event->wd), mask(event->mask), cookie(event->cookie)
{
    if(event->len)
    {
        name.assign(event->name, strnlen(event->name, event->len-1));
    }
}

INotify::INotify(const size_t& maxBufferBytes) : buffer(maxBufferBytes), mNotifyFd(inotify_init())
{
    if(mNotifyFd == AutoFD::NONE)
    {
        throw EINotify("inotify_init");
    }

    strncpy(mKickerPath, "/tmp/inotify-XXXXXX", sizeof(mKickerPath));
    mKickerFd = mkstemp(mKickerPath);

    if(mKickerFd == AutoFD::NONE)
    {
        throw EINotify("mkstemp");
    }

    mKickerWatchFd = AddWatch(mKickerPath, IN_MODIFY | IN_ATTRIB);

    mWatchFds.insert(mKickerWatchFd);

    if(mKickerWatchFd == AutoFD::NONE)
    {
        throw EINotify("mKickerWatchFd");
    }
}

INotify::~INotify()
{
    Interrupt();
    while(! mWatchFds.empty())
    {
        const std::set<int>::value_type fd(*mWatchFds.begin());
        const int watchFd(inotify_rm_watch(mNotifyFd, fd));

        if(watchFd < 0)
        {
            hlog(HLOG_ERROR, "inotify_rm_watch error: %d", watchFd);
        }

        mWatchFds.erase(fd);
    }

    FileSystem fs;
    fs.Unlink(mKickerPath);
}

int INotify::AddWatch(const std::string& path, const int& mask)
{
    AutoUnlockMutex guard(mMutex);
    const int watchFd(inotify_add_watch(mNotifyFd, path.c_str(), mask));

    if(watchFd < 0)
    {
        throw EINotifyAddWatchFailed(FStringFC(),"inotify_add_watch error: %d", watchFd);
    }

    mWatchFds.insert(watchFd);
    return watchFd;
}

void INotify::RemoveWatch(const int& fd)
{
    AutoUnlockMutex guard(mMutex);
    const int watchFd(inotify_rm_watch(mNotifyFd, fd));

    if(watchFd < 0)
    {
        throw EINotifyRemoveWatchFailed(FStringFC(),"inotify_rm_watch error: %d", watchFd);
    }

    mWatchFds.erase(fd);
}

INotify::EventVector INotify::Read(const time_t& secs)
{
    AutoUnlockMutex guard(mMutex);

    INotify::EventVector events;

    timeval timeout = {secs,0};
    fd_set set;
    FD_ZERO (&set);
    FD_SET (mNotifyFd, &set);

    const int numDescriptors(select (FD_SETSIZE,&set, NULL, NULL, &timeout));

    if(numDescriptors > 0)
    {
        const size_t length(read(mNotifyFd, buffer.data(), buffer.size()));

        for (size_t i=0; i < length;)
        {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];

            if (event->wd != mKickerWatchFd)
            {
                events.push_back(make_shared<Event>(event));
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }
    else if(numDescriptors < 0)
    {
        if(errno != EINTR)
        {
            const FString error("select error: %d", errno);
            hlog(HLOG_ERROR, "%s", error.c_str());
        }
    }

    return events;
}

void INotify::Interrupt()
{
    FileSystem fs;
    fs.Touch(mKickerPath);
}

namespace Forte
{

std::ostream& operator<<(std::ostream& out, const INotify::Event& o)
{
    out << "Inotify::Event {wd=" << o.wd << ", mask=" << o.mask;

    if(o.cookie)
    {
        out << ", cookie=" << o.cookie;
    }

    if(! o.name.empty())
    {
        out << ", name=" << o.name;
    }

    out << "}";

    return out;
}

}
