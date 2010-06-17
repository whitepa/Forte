#ifndef _Exception_h
#define _Exception_h

#include "Object.h"
#include "FString.h"
#include <cstdarg>
#include <list>
#include <vector>


#define EXCEPTION_CLASS(NAME)                                           \
    class NAME : public Forte::Exception                                \
    {                                                                   \
    public:                                                             \
        inline NAME() {}                                                \
        inline NAME(const char *description) :                          \
            Exception(description) {}                                   \
        inline NAME(const Forte::FStringFC &fc, const char *format, ...) \
            __attribute__((format(printf,3,4)))                         \
        {                                                               \
            va_list ap;                                                 \
            int size;                                                   \
            va_start(ap, format);                                       \
            size = vsnprintf(NULL, 0, format, ap);                      \
            va_end(ap);                                                 \
                                                                        \
            va_start(ap, format);                                       \
            mDescription.VFormat(format, size, ap);                     \
            va_end(ap);                                                 \
        }                                                               \
    };

#define EXCEPTION_SUBCLASS(PARENT, NAME)                                \
    class NAME : public PARENT                                          \
    {                                                                   \
    public:                                                             \
        inline NAME() {}                                                \
        inline NAME(const char *description) : PARENT(description) {}   \
        inline NAME(const Forte::FStringFC &fc, const char *format, ...) \
            __attribute__((format(printf,3,4)))                         \
        {                                                               \
            va_list ap;                                                 \
            int size;                                                   \
            va_start(ap, format);                                       \
            size = vsnprintf(NULL, 0, format, ap);                      \
            va_end(ap);                                                 \
                                                                        \
            va_start(ap, format);                                       \
            mDescription.VFormat(format, size, ap);                     \
            va_end(ap);                                                 \
        }                                                               \
    };

#define EXCEPTION_SUBCLASS2(PARENT, NAME, DESC)                         \
    class NAME : public PARENT                                          \
    {                                                                   \
    public:                                                             \
        inline NAME() : PARENT(DESC) {}                                 \
        inline NAME(const char *description) :                          \
            PARENT(Forte::FStringFC(), "%s: %s", DESC, description) {}  \
        inline NAME(const Forte::FStringFC &fc,                         \
                    const char *format, ...)                            \
            __attribute__((format(printf,3,4))) :                       \
            PARENT(DESC)                                                \
        {                                                               \
            va_list ap;                                                 \
            int size;                                                   \
            va_start(ap, format);                                       \
            size = vsnprintf(NULL, 0, format, ap);                      \
            va_end(ap);                                                 \
                                                                        \
            Forte::FString tmp;                                         \
            va_start(ap, format);                                       \
            tmp.VFormat(format, size, ap);                              \
            va_end(ap);                                                 \
                                                                        \
            mDescription += ": " + tmp;                                 \
        }                                                               \
    };


namespace Forte
{
    class Exception : public Object
    {
    public:
        Exception();
        Exception(const char *description);
        Exception(const FStringFC &fc, const char *format, ...)  __attribute__((format(printf,3,4)));
        Exception(const Exception& other);
        virtual ~Exception() throw();
        inline std::string &GetDescription() { return mDescription; };
        inline std::string &What() { return mDescription; };
        inline std::string &what() { return mDescription; };
        std::string ExtendedDescription();
        FString mDescription;
        std::list<void *> mStack;
    };


    EXCEPTION_SUBCLASS2(Exception, EEmptyReference, "Empty Reference");
    EXCEPTION_SUBCLASS2(Exception, EUnimplemented, "Unimplemented");
    EXCEPTION_SUBCLASS2(Exception, EUninitialized, "Uninitialized");

    // FString is used in Exception, so FString cannot include
    // Exception.h easily. best thing to do is just declare the FString
    // exceptions here
    EXCEPTION_SUBCLASS2(Exception, EFString, "FString");
    EXCEPTION_SUBCLASS2(EFString, EFStringLoadFile, "LoadFile");
    EXCEPTION_SUBCLASS2(EFString, EFStringUnknownAddressFamily, 
                        "Unknown address family");
};
#endif
