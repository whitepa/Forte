#include "FileSystemUtil.h"

using namespace Forte;


FileSystemUtil::FileSystemUtil(ProcRunner &procRunner) : mProcRunner(procRunner)
{
    FTRACE;
}

FileSystemUtil::~FileSystemUtil()
{
}

void FileSystemUtil::Mount(const FString&  filesystemType,
                           const FString&  devicePath,
                           const FString&  mountPath,
                           const FString&  options)
{
    FTRACE2("%s, %s, %s, %s", filesystemType.c_str(), devicePath.c_str(),
            mountPath.c_str(), options.c_str());

    FString  cmd(FStringFC(), "/bin/mount -o '%s' -t %s %s %s",
                 options.c_str(),
                 filesystemType.c_str(),
                 devicePath.c_str(),
                 mountPath.c_str());
    FString  output;
    int ret;

    if ((ret = mProcRunner.Run(cmd, "", &output, 
                               PROC_RUNNER_DEFAULT_TIMEOUT)) != 0)
    {
        hlog(HLOG_ERROR, "Unable to run '%s' [return code: %i] (%s)",
             cmd.c_str(), ret, output.c_str());
        throw EDeviceMount(FStringFC(), "Unable to mount '%s' on '%s' (%s)",
                           devicePath.c_str(), mountPath.c_str(),
                           filesystemType.c_str());
    }

}

void FileSystemUtil::Unmount(const FString&  mountPath)
{
    FTRACE2("%s", mountPath.c_str());

    FString  cmd(FStringFC(), "/bin/umount %s", mountPath.c_str());
    FString  output;
    int ret;

    if ((ret = mProcRunner.Run(cmd, "", &output, 
                               PROC_RUNNER_DEFAULT_TIMEOUT)) != 0)
    {
        hlog(HLOG_ERROR, "Unable to run '%s' [return code: %i] (%s)",
             cmd.c_str(), ret, output.c_str());
        throw EDeviceUnmount(FStringFC(), "Unable to unmount '%s'",
                            mountPath.c_str());
    }
}


void FileSystemUtil::Format(const FString& devicePath, const FString& type, 
                            bool force)
{
    FTRACE2("%s, %s", devicePath.c_str(), type.c_str());
    
    FString cmd(FStringFC(), "/sbin/mkfs.%s %s %s", type.c_str(),
                (force ? "-F" : ""), devicePath.c_str());
    FString output;
    int rtnCode;

    if ((rtnCode = mProcRunner.Run(cmd, "", &output, 
                                   PROC_RUNNER_DEFAULT_TIMEOUT)) != 0)
    {
        hlog(HLOG_ERROR, "Unable to run '%s' [return code: %i] (%s)",
             cmd.c_str(), rtnCode, output.c_str());
        throw EDeviceFormat(FStringFC(), 
                            "Unable to format '%s' [type: %s]",
                            devicePath.c_str(), type.c_str());
    }
}
