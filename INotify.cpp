#include "INotify.h"
#include "FileSystemImpl.h"
#include <sys/inotify.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace Forte;
using namespace boost;
using namespace std;

INotify::Event::Event(const inotify_event* event)
    : wd(event->wd), mask(event->mask), cookie(event->cookie), name(event->len ? event->name: "")
{
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
            hlogstream (HLOG_ERROR, "inotify_rm_watch on " << fd << " with error: " << watchFd);
        }

        mWatchFds.erase(fd);
    }

    FileSystemImpl fs;
    fs.Unlink(mKickerPath);
}

int INotify::AddWatch(const std::string& path, const int& mask)
{
    AutoUnlockMutex guard(mMutex);

    const int watchFd(inotify_add_watch(mNotifyFd, path.c_str(), mask));

    if(watchFd < 0)
    {
        throw EINotifyAddWatchFailed(FStringFC(),
                                     "inotify_add_watch error: %d on %s",
                                     watchFd, path.c_str());
    }

    mWatchFds.insert(watchFd);

    hlogstream (HLOG_DEBUG, "added watch " << watchFd << " on '" << path << "' with mask: " << hex << showbase << mask);

    return watchFd;
}

void INotify::RemoveWatch(const int& fd)
{
    AutoUnlockMutex guard(mMutex);

    set<int>::iterator i = mWatchFds.find(fd);

    if (i == mWatchFds.end())
    {
        throw EINotifyRemoveWatchFailed(FStringFC(),"%d not watched", fd);
    }

    const int watchFd(inotify_rm_watch(mNotifyFd, fd));

    if(watchFd < 0)
    {
        throw EINotifyRemoveWatchFailed(FStringFC(),"inotify_rm_watch error: %d on %d", watchFd, fd);
    }

    mWatchFds.erase(i);

    hlogstream (HLOG_DEBUG, "removing watch " << fd);
}

bool INotify::IsWatch(const int& fd) const
{
    return (mWatchFds.find(fd) != mWatchFds.end());
}

INotify::EventVector INotify::Read(const unsigned long& ms)
{
    AutoUnlockMutex guard(mMutex);

    INotify::EventVector events;

    timeval timeout = {ms/1000, (ms%1000)*1000};

    if (timeout.tv_usec > 1000000)
    {
        timeout.tv_usec -= 1000000;
        ++timeout.tv_sec;
    }

    fd_set set;
    FD_ZERO (&set);
    FD_SET (mNotifyFd, &set);

    const int numDescriptors(select (FD_SETSIZE,&set, NULL, NULL, &timeout));

    if(numDescriptors > 0)
    {
        const ssize_t length(read(mNotifyFd, buffer.data(), buffer.size()));

        if (length != -1)
        {
            for (ssize_t i=0; i < length;)
            {
                struct inotify_event *event = (struct inotify_event *) &buffer[i];

                if (event->wd != mKickerWatchFd)
                {
                    events.push_back(make_shared<Event>(event));
                }

                if (event->mask & IN_IGNORED)
                {
                    mWatchFds.erase(event->wd);
                }

                i += sizeof(struct inotify_event) + event->len;
            }
        }
        else
        {
            if (errno != EINTR)
            {
                hlogstream (HLOG_ERROR, "read error: " << errno);
            }
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
    FileSystemImpl fs;
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
