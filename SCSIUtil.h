#ifndef __forte_SCSIUtil_h
#define __forte_SCSIUtil_h

#include "ProcRunner.h"
#include "FileSystem.h"
// _____________________________________________________________________________

typedef struct _scsi_idlun
{
    int  info;    /* 4 separate bytes of info compacted into 1 int */
                  // TODO: 1 byte/field => 256 values max for each field
                  //       to find out, if that's as per T10 spec?

    int  host_unique_id; /* distinguishes adapter cards from same supplier */
} scsi_idlun;
// _____________________________________________________________________________

namespace Forte
{
    EXCEPTION_CLASS(ESCSIUtil);
    EXCEPTION_SUBCLASS(ESCSIUtil, ESCSIUtilDeviceOpenFailed);
    EXCEPTION_SUBCLASS(ESCSIUtil, ESCSIUtilioctlFailed);
    EXCEPTION_SUBCLASS(ESCSIUtil, ESCSIScanFailed);

    class SCSIUtil : public Object
    {
    public:
        SCSIUtil(ProcRunner& procRunner, FileSystem& fs);
        virtual ~SCSIUtil(void);

        /**
         * Using ioctl() get host-id & lun-id for the given SCSI device
         * @param[in]  devicePath   path to the device (e.g., /dev/sda)
         * @param[out] slotId       slotId i.e. host-id for the given device
         * @param[out] lunId        lun-id for the given device
         * @throws ESCSIUtilDeviceOpenFailed, ESCSIUtilioctlFailed
         */
        void GetDeviceInfo(const FString& devicePath,
                           int&           slotId,
                           int&           lunId);

        /**
         * Re-scans the SCSI host.
         * @param hostId     the host id to scan
         * @throws ESCSIScanFailed
         */
        void RescanHost(int hostId);

        /**
         * Re-scans the all SCSI hosts it can find.
         * @throws ESCSIScanFailed on any host it finds that it cannot scan
         */
        void RescanAllHosts();

    protected:
        ProcRunner&  mProcRunner;
        FileSystem&  mFileSystem;
    };
};
// _____________________________________________________________________________

#endif
