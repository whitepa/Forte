#ifndef _Exception_h
#define _Exception_h

#include "FString.h"
#include <cstdarg>
#include <list>
#include <vector>



class CException
{
 public:
    CException();
    CException(const char *description);
    CException(const FStringFC &fc, const char *format, ...)  __attribute__((format(printf,3,4)));
    CException(const CException& other);
    virtual ~CException() throw();
    inline std::string &getDescription() { return mDescription; };
    inline std::string &what() { return mDescription; };
    std::string extendedDescription();
    FString mDescription;
    std::list<void *> mStack;
    void pretty_trace_log(int log_level);
 private:
    FString pretty_frame(const std::vector<FString>& mapping, void *address);
};

#define EXCEPTION_CLASS(NAME)                                           \
    class NAME : public CException                                      \
    {                                                                   \
    public:                                                             \
        inline NAME() {}                                                \
        inline NAME(const char *description) :                          \
            CException(description) {}                                  \
        inline NAME(const FStringFC &fc, const char *format, ...)       \
            __attribute__((format(printf,3,4)))                         \
        {                                                               \
            va_list ap;                                                 \
            int size;                                                   \
            va_start(ap, format);                                       \
            size = vsnprintf(NULL, 0, format, ap);                      \
            va_end(ap);                                                 \
                                                                        \
            va_start(ap, format);                                       \
            mDescription.vFormat(format, size, ap);                     \
            va_end(ap);                                                 \
        }                                                               \
    }

#define EXCEPTION_SUBCLASS(PARENT, NAME)                                \
    class NAME : public PARENT                                          \
    {                                                                   \
    public:                                                             \
        inline NAME() {}                                                \
        inline NAME(const char *description) : PARENT(description) {}   \
        inline NAME(const FStringFC &fc, const char *format, ...)       \
            __attribute__((format(printf,3,4)))                         \
        {                                                               \
            va_list ap;                                                 \
            int size;                                                   \
            va_start(ap, format);                                       \
            size = vsnprintf(NULL, 0, format, ap);                      \
            va_end(ap);                                                 \
                                                                        \
            va_start(ap, format);                                       \
            mDescription.vFormat(format, size, ap);                     \
            va_end(ap);                                                 \
        }                                                               \
    }

#define EXCEPTION_SUBCLASS2(PARENT, NAME, DESC)                         \
    class NAME : public PARENT                                          \
    {                                                                   \
    public:                                                             \
        inline NAME() : PARENT(DESC) {}                                \
        inline NAME(const char *description) :                          \
            PARENT(FStringFC(), "%s: %s", DESC, description) {}        \
        inline NAME(const FStringFC &fc, const char *format, ...)       \
            __attribute__((format(printf,3,4))) :                       \
                PARENT(DESC)                                           \
        {                                                               \
            va_list ap;                                                 \
            int size;                                                   \
            va_start(ap, format);                                       \
            size = vsnprintf(NULL, 0, format, ap);                      \
            va_end(ap);                                                 \
                                                                        \
            FString tmp;                                                \
            va_start(ap, format);                                       \
            tmp.vFormat(format, size, ap);                              \
            va_end(ap);                                                 \
                                                                        \
            mDescription += ": " + tmp;                                 \
        }                                                               \
    }


EXCEPTION_SUBCLASS(CException, CForteException);
EXCEPTION_SUBCLASS2(CForteException, CForteEmptyReferenceException, "Empty Reference");
EXCEPTION_SUBCLASS2(CForteException, CForteUnimplementedException, "Unimplemented");
EXCEPTION_SUBCLASS2(CForteException, CForteUninitializedException, "Uninitialized");
EXCEPTION_SUBCLASS2(CForteException, EUnimplemented, "Unimplemented");


// FString is used in CException, so FString cannot include
// Exception.h easily. best thing to do is just declare the FString
// exceptions here
EXCEPTION_SUBCLASS2(CForteException, CForteFStringException, "FString");
EXCEPTION_SUBCLASS2(CForteFStringException, CForteFStringLoadFileException, "LoadFile");
EXCEPTION_SUBCLASS2(CForteFStringException, CForteFStringUnknownAddressFamily, "Unknown address family");
#endif
