#ifndef __forte_autodynamiclibraryhandle_h
#define __forte_autodynamiclibraryhandle_h

#include <dlfcn.h>
#include "Exception.h"

EXCEPTION_CLASS(EDynamicLoadLibrary);
EXCEPTION_SUBCLASS(EDynamicLoadLibrary, EUnableToLoadLibrary);
EXCEPTION_SUBCLASS(EDynamicLoadLibrary, EUnableToLoadFunction);
EXCEPTION_SUBCLASS(EDynamicLoadLibrary, ELibraryNotLoaded);

namespace Forte
{
   /**
    * Automatically close dynamic loaded library handles
    */
    class AutoDynamicLibraryHandle
    {
    public:
        AutoDynamicLibraryHandle(const Forte::FString &path) : mHandle(NULL), mPath(path) {
            FTRACE2("%s", path.c_str());
        }

        virtual void Load() {
            mHandle = dlopen (mPath.c_str(), RTLD_LAZY);
            if (!mHandle)
            {
                hlog(HLOG_ERR, "Error loading '%s':\n%s", mPath.c_str(),
                     dlerror());
                throw EUnableToLoadLibrary(FStringFC(),
                                           "Error loading '%s'", mPath.c_str());
            }
            dlerror();    // Clear any existing error
        }

        virtual ~AutoDynamicLibraryHandle() {
            if (mHandle != NULL)
            {
                dlclose(mHandle);
            }
        }

        virtual bool IsLoaded() {
            return (mHandle != NULL);
        }

        virtual void *GetFunctionPointer(const Forte::FString& fName) {
            char *error = NULL;
            void *func = NULL;

            if (!IsLoaded())
            {
                hlog(HLOG_ERR, "Unable to get pointer to %s because the library %s is not loaded", fName.c_str(), mPath.c_str());
                throw ELibraryNotLoaded(FStringFC(), "Unable to get pointer to %s because the library %s is not loaded", fName.c_str(), mPath.c_str());
            }

            dlerror(); //clear any existing error

            func = dlsym(mHandle, fName.c_str());
            if ((error = dlerror()) != NULL)
            {
                hlog(HLOG_ERR, "Error loading symbol for '%s' in '%s':\n%s",
                     mPath.c_str(), fName.c_str(), error);
                throw EUnableToLoadFunction(FStringFC(), "Error loading symbol for '%s' in '%s':\n%s", mPath.c_str(), fName.c_str(), error);
            }

            return func;
        }

        void *mHandle;
        Forte::FString mPath;
    };
    typedef boost::shared_ptr<AutoDynamicLibraryHandle> AutoDynamicLibraryHandlePtr;

    class AutoDynamicLibraryHandleFactory
    {
    public:
        AutoDynamicLibraryHandleFactory() {}
        virtual ~AutoDynamicLibraryHandleFactory() {}
        virtual AutoDynamicLibraryHandlePtr Get(const Forte::FString& path) const {
            FTRACE2("%s", path.c_str());
            AutoDynamicLibraryHandlePtr autoHandle(
                new AutoDynamicLibraryHandle(path));
            autoHandle->Load();
            return autoHandle;
        }
    };
};


#endif
