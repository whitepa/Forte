#ifndef __SystemCallUtil_h
#define __SystemCallUtil_h

#include "Exception.h"

EXCEPTION_CLASS(ESystemError);

// from /usr/include/asm-generic/errno-base.h
EXCEPTION_SUBCLASS(ESystemError, EErrNoEPERM);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENOENT);
EXCEPTION_SUBCLASS(ESystemError, EErrNoESRCH);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEINTR);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEIO);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENXIO);
EXCEPTION_SUBCLASS(ESystemError, EErrNoE2BIG);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENOEXEC);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEBADF);
EXCEPTION_SUBCLASS(ESystemError, EErrNoECHILD);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEAGAIN);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENOMEM);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEACCES);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEFAULT);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENOTBLK);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEBUSY);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEEXIST);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEXDEV);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENODEV);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENOTDIR);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEISDIR);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEINVAL);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENFILE);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEMFILE);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENOTTY);
EXCEPTION_SUBCLASS(ESystemError, EErrNoETXTBSY);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEFBIG);
EXCEPTION_SUBCLASS(ESystemError, EErrNoENOSPC);
EXCEPTION_SUBCLASS(ESystemError, EErrNoESPIPE);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEROFS);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEMLINK);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEPIPE);
EXCEPTION_SUBCLASS(ESystemError, EErrNoEDOM);
EXCEPTION_SUBCLASS(ESystemError, EErrNoERANGE);

// TODO: /usr/include/asm-generic/errno.h

namespace Forte
{
    class SystemCallUtil
    {
    public:
        static void ThrowErrNoException(int theErrNo) {
            ThrowErrNoException(theErrNo, GetErrorDescription(theErrNo));
        }

        static void ThrowErrNoException(int theErrNo, const FString& errDesc) {
            hlog(HLOG_ERR, "errno %d (%s)", theErrNo, errDesc.c_str()); 

            switch (theErrNo) {
            case 0:
                throw ESystemError("Unexpected errno=0");
            case EPERM:
                throw EErrNoEPERM(errDesc);
            case ENOENT:
                throw EErrNoENOENT(errDesc);
            case ESRCH:
                throw EErrNoESRCH(errDesc);
            case EINTR:
                throw EErrNoEINTR(errDesc);
            case EIO:
                throw EErrNoEIO(errDesc);
            case ENXIO:
                throw EErrNoENXIO(errDesc);
            case E2BIG:
                throw EErrNoE2BIG(errDesc);
            case ENOEXEC:
                throw EErrNoENOEXEC(errDesc);
            case EBADF:
                throw EErrNoEBADF(errDesc);
            case ECHILD:
                throw EErrNoECHILD(errDesc);
            case EAGAIN:
                throw EErrNoEAGAIN(errDesc);
            case ENOMEM:
                throw EErrNoENOMEM(errDesc);
            case EACCES:
                throw EErrNoEACCES(errDesc);
            case EFAULT:
                throw EErrNoEFAULT(errDesc);
            case ENOTBLK:
                throw EErrNoENOTBLK(errDesc);
            case EBUSY:
                throw EErrNoEBUSY(errDesc);
            case EEXIST:
                throw EErrNoEEXIST(errDesc);
            case EXDEV:
                throw EErrNoEXDEV(errDesc);
            case ENODEV:
                throw EErrNoENODEV(errDesc);
            case ENOTDIR:
                throw EErrNoENOTDIR(errDesc);
            case EISDIR:
                throw EErrNoEISDIR(errDesc);
            case EINVAL:
                throw EErrNoEINVAL(errDesc);
            case ENFILE:
                throw EErrNoENFILE(errDesc);
            case EMFILE:
                throw EErrNoEMFILE(errDesc);
            case ENOTTY:
                throw EErrNoENOTTY(errDesc);
            case ETXTBSY:
                throw EErrNoETXTBSY(errDesc);
            case EFBIG:
                throw EErrNoEFBIG(errDesc);
            case ENOSPC:
                throw EErrNoENOSPC(errDesc);
            case ESPIPE:
                throw EErrNoESPIPE(errDesc);
            case EROFS:
                throw EErrNoEROFS(errDesc);
            case EMLINK:
                throw EErrNoEMLINK(errDesc);
            case EPIPE:
                throw EErrNoEPIPE(errDesc);
            case EDOM:
                throw EErrNoEDOM(errDesc);
            case ERANGE:
                throw EErrNoERANGE(errDesc);
            default:
                throw ESystemError(errDesc);
            }
        }

        static FString GetErrorDescription(int err)  {
            char buf[256], *str;
            FString ret;
            str = strerror_r(err, buf, sizeof(buf));
            buf[sizeof(buf) - 1] = 0;
            ret = str;
            return ret;
        }
    };
};

#endif
