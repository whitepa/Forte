#ifndef __SystemErrorException_h
#define __SystemErrorException_h

#include "Exception.h"

EXCEPTION_CLASS(ESystemError);

EXCEPTION_SUBCLASS2(ESystemError, EErrNoEPERM, "Operation not permitted");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENOENT, "No such file or directory");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoESRCH, "No such process");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEINTR, "Interrupted system call");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEIO, "I/O error");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENXIO, "No such device or address");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoE2BIG, "Argument list too long");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENOEXEC, "Exec format error");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEBADF, "Bad file number");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoECHILD, "No child processes");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEAGAIN, "Try again");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENOMEM, "Out of memory");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEACCES, "Permission denied");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEFAULT, "Bad address");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENOTBLK, "Block device required");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEBUSY, "Device or resource busy");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEEXIST, "File exists");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEXDEV, "Cross-device link");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENODEV, "No such device");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENOTDIR, "Not a directory");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEISDIR, "Is a directory");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEINVAL, "Invalid argument");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENFILE, "File table overflow");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEMFILE, "Too many open files");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENOTTY, "Not a typewriter");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoETXTBSY, "Text file busy");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEFBIG, "File too large");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoENOSPC, "No space left on device");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoESPIPE, "Illegal seek");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEROFS, "Read-only file system");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEMLINK, "Too many links");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEPIPE, "Broken pipe");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoEDOM, "Math argument out of domain of func");
EXCEPTION_SUBCLASS2(ESystemError, EErrNoERANGE, "Math result not representable");

#define THROW_ERRNO_EXCEPTION(_DESC)            \
    switch (errno) {                            \
    case 0:                                     \
        break;                                  \
    case EPERM:                                 \
        throw EErrNoEPERM(_DESC);               \
    case ENOENT:                                \
        throw EErrNoENOENT(_DESC);              \
    case ESRCH:                                 \
        throw EErrNoESRCH(_DESC);               \
    case EINTR:                                 \
        throw EErrNoEINTR(_DESC);               \
    case EIO:                                   \
        throw EErrNoEIO(_DESC);                 \
    case ENXIO:                                 \
        throw EErrNoENXIO(_DESC);               \
    case E2BIG:                                 \
        throw EErrNoE2BIG(_DESC);               \
    case ENOEXEC:                               \
        throw EErrNoENOEXEC(_DESC);             \
    case EBADF:                                 \
        throw EErrNoEBADF(_DESC);               \
    case ECHILD:                                \
        throw EErrNoECHILD(_DESC);              \
    case EAGAIN:                                \
        throw EErrNoEAGAIN(_DESC);              \
    case ENOMEM:                                \
        throw EErrNoENOMEM(_DESC);              \
    case EACCES:                                \
        throw EErrNoEACCES(_DESC);              \
    case EFAULT:                                \
        throw EErrNoEFAULT(_DESC);              \
    case ENOTBLK:                               \
        throw EErrNoENOTBLK(_DESC);             \
    case EBUSY:                                 \
        throw EErrNoEBUSY(_DESC);               \
    case EEXIST:                                \
        throw EErrNoEEXIST(_DESC);              \
    case EXDEV:                                 \
        throw EErrNoEXDEV(_DESC);               \
    case ENODEV:                                \
        throw EErrNoENODEV(_DESC);              \
    case ENOTDIR:                               \
        throw EErrNoENOTDIR(_DESC);             \
    case EISDIR:                                \
        throw EErrNoEISDIR(_DESC);              \
    case EINVAL:                                \
        throw EErrNoEINVAL(_DESC);              \
    case ENFILE:                                \
        throw EErrNoENFILE(_DESC);              \
    case EMFILE:                                \
        throw EErrNoEMFILE(_DESC);              \
    case ENOTTY:                                \
        throw EErrNoENOTTY(_DESC);              \
    case ETXTBSY:                               \
        throw EErrNoETXTBSY(_DESC);             \
    case EFBIG:                                 \
        throw EErrNoEFBIG(_DESC);               \
    case ENOSPC:                                \
        throw EErrNoENOSPC(_DESC);              \
    case ESPIPE:                                \
        throw EErrNoESPIPE(_DESC);              \
    case EROFS:                                 \
        throw EErrNoEROFS(_DESC);               \
    case EMLINK:                                \
        throw EErrNoEMLINK(_DESC);              \
    case EPIPE:                                 \
        throw EErrNoEPIPE(_DESC);               \
    case EDOM:                                  \
        throw EErrNoEDOM(_DESC);                \
    case ERANGE:                                \
        throw EErrNoERANGE(_DESC);              \
    default:                                    \
        throw ESystemError(_DESC);              \
    }

#endif
