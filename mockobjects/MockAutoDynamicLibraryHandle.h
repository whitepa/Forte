#ifndef _fortemock_MockAutoDynamicLibraryHandle_h
#define _fortemock_MockAutoDynamicLibraryHandle_h

#include "AutoDynamicLibraryHandle.h"

namespace Forte
{
    class MockAutoDynamicLibraryHandle : public AutoDynamicLibraryHandle
    {
    public:
        bool mLoaded;
        MockAutoDynamicLibraryHandle(const Forte::FString &path) : AutoDynamicLibraryHandle(path), mLoaded(false) {
            FTRACE;
        }
        
        virtual ~MockAutoDynamicLibraryHandle() {}

        void Load() {
            mLoaded = true;
        }
        
        bool IsLoaded() { return mLoaded; }
        void *GetFunctionPointer(const Forte::FString& fName) {
            if (!mLoaded)
            {
                throw ELibraryNotLoaded(mPath);
            }
            return NULL;
        }
    };

    typedef boost::shared_ptr<MockAutoDynamicLibraryHandle> MockAutoDynamicLibraryHandlePtr;

    class MockAutoDynamicLibraryHandleFactory : public AutoDynamicLibraryHandleFactory
    {
    public:
        MockAutoDynamicLibraryHandleFactory() {}
        ~MockAutoDynamicLibraryHandleFactory() {}
        AutoDynamicLibraryHandlePtr Get(const Forte::FString& path) const {
            FTRACE2("%s", path.c_str());
            MockAutoDynamicLibraryHandlePtr autoHandle(
                new MockAutoDynamicLibraryHandle(path));
            return autoHandle;
        }
    };


};

#endif
