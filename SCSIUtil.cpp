#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <scsi/scsi.h>

#include "FTrace.h"
#include "SCSIUtil.h"

using namespace Forte;
// _____________________________________________________________________________

SCSIUtil::SCSIUtil(ProcRunner&  procRunner)
: mProcRunner(procRunner)
{
    FTRACE;
}
// _____________________________________________________________________________

SCSIUtil::~SCSIUtil(void)
{
}
// _____________________________________________________________________________

void SCSIUtil::GetDeviceInfo(const FString&  devicePath,
                             int&            hostId,
                             int&            lunId)
{
    FTRACE2("%s", devicePath.c_str());

    int  fd;

    if ((fd = open(devicePath.c_str(), O_RDONLY)) < 0)
    {
        /* Note that most SCSI commands require the O_RDWR flag to be set */
        hlog(HLOG_ERROR, "Failed to open '%s' [errno: %d]",
             devicePath.c_str(), errno);
        throw ESCSIUtil(FStringFC(), "Failed to open '%s'", devicePath.c_str());
    }

    scsi_idlun  s;

    if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &s) < 0)
    {
        hlog(HLOG_ERROR, "ioctl(SCSI_IOCTL_GET_IDLUN) failed ... [errno: %d]", errno);
        throw ESCSIUtil(FStringFC(),
                        "ioctl(SCSI_IOCTL_GET_IDLUN) failed ... [errno: %d]",
                        errno);
    }

    hostId = (s.info >> 24) & 0x000000FF,
    lunId  = (s.info >>  8) & 0x000000FF;

    hlog(HLOG_DEBUG, "host: %d, channel: %d, target: %d, lun: %d",
            hostId,
            (s.info >> 16) & 0x000000FF,
             s.info        & 0x000000FF,
            lunId);

    close(fd);

} /* SCSIUtil::GetDeviceInfo() */
// _____________________________________________________________________________
