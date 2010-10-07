#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <scsi/scsi.h>

#include "FTrace.h"
#include "SCSIUtil.h"

#define  ERR_BUF_LEN    256

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

    int  fd;

    if ((fd = open(devicePath.c_str(), O_RDONLY)) < 0)
    { /* Note that most SCSI commands require the O_RDWR flag to be set */

        char  err_buf[ERR_BUF_LEN];

        memset(err_buf, 0, ERR_BUF_LEN);
        strerror_r(errno, err_buf, ERR_BUF_LEN-1);

        hlog(HLOG_ERROR, "Failed to open '%s' [errno: %d]",
                         devicePath.c_str(), errno);
        hlog(HLOG_ERROR, "strerror: %s", err_buf);

        throw ESCSIUtilDeviceOpenFailed(FStringFC(), "Failed to open '%s'",
                                                     devicePath.c_str());
    }

    scsi_idlun  s;

    if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &s) < 0)
    {
        close(fd);

        char  err_buf[ERR_BUF_LEN];

        memset(err_buf, 0, ERR_BUF_LEN);
        strerror_r(errno, err_buf, ERR_BUF_LEN-1);

        hlog(HLOG_ERROR, "ioctl(SCSI_IOCTL_GET_IDLUN) failed ... [errno: %d]",
                         errno);
        hlog(HLOG_ERROR, "strerror: %s", err_buf);

        throw ESCSIUtilioctlFailed
              (FStringFC(),
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
// -----------------------------------------------------------------------------

void SCSIUtil::RescanHost(int hostId)
{
    FTRACE2("%i", hostId);

    FString output;
    FString cmd(FStringFC(), 
                "/bin/echo - - - > /sys/class/scsi_host/host%i/scan", hostId);
    if (mProcRunner.Run(cmd, "", &output, PROC_RUNNER_NO_TIMEOUT) != 0)
    {
        hlog(HLOG_ERR, "Unable to rescan the scsi host %i (%s)\n%s",
             hostId, cmd.c_str(), output.c_str());
        throw ESCSIScanFailed();
    }
}
// -----------------------------------------------------------------------------
