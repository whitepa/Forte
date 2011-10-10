#ifndef _Exception_h
#define _Exception_h

#include "Object.h"
#include "FString.h"
#include <cstdarg>
#include <list>
#include <vector>
#include <exception>
#include <boost/exception/all.hpp>
#include <boost/regex.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#define PARAMETER_NAME_DATA(z, n, data) BOOST_PP_CAT(parameter, BOOST_PP_SUB(n, 2))
#define PARAMETER_NAME(z, n, data) BOOST_PP_CAT(parameter, n)
#define PARAMETER_TYPES_AND_NAMES_REFERENCE(s, data, elem) const elem& PARAMETER_NAME_DATA(~, s, ~)

#define EXCEPTION_PARAM_SUBCLASS(PARENT, NAME, DESC, PARAMS_SEQ)                                        \
    class NAME : public virtual PARENT,                                                                 \
                 public Forte::ParameterizedException< BOOST_PP_SEQ_ENUM(PARAMS_SEQ) >                 \
    {                                                                                                   \
    public:                                                                                             \
        inline NAME(BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(PARAMETER_TYPES_AND_NAMES_REFERENCE, 0, PARAMS_SEQ))) : \
            Forte::ParameterizedException< BOOST_PP_SEQ_ENUM(PARAMS_SEQ) >(DESC, BOOST_PP_ENUM(BOOST_PP_SEQ_SIZE(PARAMS_SEQ), PARAMETER_NAME, ~))        \
        {                                                                                               \
        }                                                                                               \
    };                                                                                                  \

/**
 * @TODO figure out how to add the format attributes back into the
 * Format constructors.  Since the addition of the dual virtual
 * inheritance, the additional implicit vtable pointer parameters seem
 * to have broken the attribute.
 *
 */
        //            __attribute__((format(printf,3,4)))               

#define EXCEPTION_CLASS(NAME)                                           \
    class NAME : public Forte::Exception                                \
    {                                                                   \
    public:                                                             \
        inline NAME() : Exception(#NAME) {}                             \
        inline NAME(const char *description) :                          \
            Exception(description) {}                                   \
        inline NAME(const Forte::FStringFC &fc, const char *format, ...) \
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
        inline NAME() : PARENT(#NAME) {}                                \
        inline NAME(const char *description) : PARENT(description) {}   \
        inline NAME(const Forte::FStringFC &fc, const char *format, ...) \
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
            :  PARENT(DESC)                                             \
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
    class Exception : public Object, 
                      public virtual std::exception,
                      public virtual boost::exception
    {
    public:
        Exception();
        Exception(const char *description);
        Exception(const FStringFC &fc, const char *format, ...);//  __attribute__((format(printf,5,6)));
        Exception(const Exception& other);
        virtual ~Exception() throw();
        const std::string &GetDescription() const throw() { 
            return mDescription; 
        }

        virtual const char *what() const throw() { 
            return mDescription; 
        }

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

    template<typename P1, typename P2>
    class ParameterizedException : public virtual Exception
    {
    public:
        ParameterizedException(const char *formattedDescription,
                const P1 &p1, const P2 &p2) :
                    Exception(FStringFC(), formattedDescription, FString(mParameter1).c_str(),
                            FString(mParameter2).c_str()),
                    mFormattedDescription(formattedDescription),
                    mParameter1(p1),
                    mParameter2(p2)
        {

        }

        virtual ~ParameterizedException()  throw()
        {
        }
        FString GetFormattedDescriptionAsObjCStyle()
        {
            const boost::regex e("%[^%]");
            return boost::regex_replace(mFormattedDescription, e, "%@");
        }
        void GetParametersAsFStrings(std::vector<FString> &result)
        {
            result.clear();
            result.push_back(FString(mParameter1));
            result.push_back(FString(mParameter2));
        }

        FString mFormattedDescription;
        const P1 mParameter1;
        const P2 mParameter2;
    };
};

class ParamException : public Forte::ParameterizedException<Forte::FString,
                Forte::FString>
{
public:
    inline ParamException(const Forte::FString& parameter0,
            const Forte::FString& parameter1) :
            Forte::ParameterizedException<Forte::FString, Forte::FString>(
                    "%s %s", parameter0, parameter1)
    {
    }
};
;

EXCEPTION_PARAM_SUBCLASS(Forte::Exception, ParaException, "%s %s", (Forte::FString)(Forte::FString));

#endif
