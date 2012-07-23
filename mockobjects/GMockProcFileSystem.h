#ifndef __gmockprocfilesystem_h
#define __gmockprocfilesystem_h

#include "ProcFileSystem.h"

namespace Forte
{
    class GMockProcFileSystem : public Forte::ProcFileSystem
    {
    public:
        GMockProcFileSystem(const FileSystemPtr &fsptr) : Forte::ProcFileSystem(fsptr) {
        }
        MOCK_METHOD1(UptimeRead, void(Forte::ProcFileSystem::Uptime&));
        MOCK_METHOD1(MemoryInfoRead, void(Forte::StrDoubleMap&));
        MOCK_METHOD1(CountOpenFileDescriptors, unsigned int(pid_t));
        MOCK_CONST_METHOD2(PidOf, void(const Forte::FString&,std::vector<pid_t>&));
        MOCK_CONST_METHOD2(ProcessIsRunning, bool(const Forte::FString&, const Forte::FString&));
        MOCK_METHOD2(SetOOMScore, void(pid_t, const Forte::FString&));
    };
    typedef boost::shared_ptr<GMockProcFileSystem> GMockProcFileSystemPtr;
};

#endif
