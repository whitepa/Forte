#ifndef __forte_FileSystemUtil_h
#define __forte_FileSystemUtil_h

#include "FileSystem.h"
#include "ProcRunner.h"
#include "FTrace.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(EFileSystem, EDeviceMount);
    EXCEPTION_SUBCLASS(EFileSystem, EDeviceUnmount);
    EXCEPTION_SUBCLASS(EFileSystem, EDeviceFormat);

    class FileSystemUtil : public Object
    {
    public:
        FileSystemUtil(ProcRunner &procRunner);
        virtual ~FileSystemUtil();

        /**
         * Use mkfs.* to create a file system on the given device path
         * @param devicePath   path to the device to format (e.g., /dev/sda1)
         * @param type         type of file system to create (e.g., ext3)
         * @param force        force the creation?
         * @throws EDeviceFormat
         */
        void Format(const FString& devicePath, const FString &type, 
                    bool force=false);

        /**
         * Mount the given device on the given mount path
         * @param filesystemType
         * @param devicePath
         * @param mountPath
         * @throws EDeviceMount when the path cannot be mounted
         */
        void Mount(const FString& filesystemType, const FString& devicePath, 
                   const FString& mountPath);
        /**
         * Unmount the given mounted path
         * @param mountPath
         * @throws EDeviceUnmount when the path cannot be unmounted
         */
        void Unmount(const FString& mountPath);
        
    protected:
              ProcRunner &mProcRunner;
    };
};

#endif
