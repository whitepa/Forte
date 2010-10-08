#include <fcntl.h>
#include <sys/ioctl.h>
#include <scsi/scsi.h>

#include "AutoFD.h"
#include "FTrace.h"
#include "SCSIUtil.h"

using namespace Forte;
// -----------------------------------------------------------------------------

SCSIUtil::SCSIUtil(ProcRunner&  procRunner)
: mProcRunner(procRunner)
{
    FTRACE;
}
// -----------------------------------------------------------------------------

SCSIUtil::~SCSIUtil(void)
{
}
// -----------------------------------------------------------------------------

void SCSIUtil::GetDeviceInfo(const FString&  devicePath,
                             int&            hostId,
                             int&            lunId)
{
    FTRACE2("%s", devicePath.c_str());

    AutoFD  fd(::open(devicePath.c_str(), O_RDONLY));
            /* Note that most SCSI commands require the O_RDWR flag to be set */

    if ((AutoFD::NONE == fd) || (fd.Fd() < 0))
    {
        hlog(HLOG_ERROR, "Failed to open '%s'", devicePath.c_str());
        hlog_errno(HLOG_ERROR);

        throw ESCSIUtilDeviceOpenFailed(FStringFC(), "Failed to open '%s'",
                                                     devicePath.c_str());
    }

    scsi_idlun  s;

    if (ioctl(fd.Fd(), SCSI_IOCTL_GET_IDLUN, &s) < 0)
    {
        hlog(HLOG_ERROR, "ioctl(SCSI_IOCTL_GET_IDLUN) failed ...");
        hlog_errno(HLOG_ERROR);

        throw ESCSIUtilioctlFailed("ioctl(SCSI_IOCTL_GET_IDLUN) failed ...");
    }

    hostId = (s.info >> 24) & 0x000000FF,
    lunId  = (s.info >>  8) & 0x000000FF;

    hlog(HLOG_DEBUG, "host: %d, channel: %d, target: %d, lun: %d",
            hostId,
            (s.info >> 16) & 0x000000FF,
             s.info        & 0x000000FF,
            lunId);

} /* SCSIUtil::GetDeviceInfo() */
// -----------------------------------------------------------------------------
