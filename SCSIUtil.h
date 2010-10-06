#ifndef __forte_SCSIUtil_h
#define __forte_SCSIUtil_h

#include "ProcRunner.h"
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
    EXCEPTION_SUBCLASS(Exception, ESCSIDeviceOpenFailed)
    EXCEPTION_SUBCLASS(Exception, ESCSIUtilioctlFailed)

    class SCSIUtil : public Object
    {
    public:
        SCSIUtil(ProcRunner& procRunner);
        virtual ~SCSIUtil(void);

        /**
         * Using ioctl() get host-id & lun-id for the given SCSI device
         * @param[in]  devicePath   path to the device (e.g., /dev/sda)
         * @param[out] hostId       host-id for the given device
         * @param[out] lunId        lun-id for the given device
         * @throws ESCSIUtil
         */
        void GetDeviceInfo(const FString& devicePath,
                           int&           hostId,
                           int&           lunId);

    protected:
        ProcRunner&  mProcRunner;
    };
};
// _____________________________________________________________________________

#endif
