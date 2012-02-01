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
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/punctuation/paren.hpp>


#define PARAMETER_APPEND_SEMICOLON(r, data, elem) elem;
#define PARAMETER_NAME_DATA(z, n, data) BOOST_PP_CAT(data, BOOST_PP_SUB(n, 2))
#define PARAMETER_INITIALIZE(s, data, elem) PARAMETER_NAME_DATA(~, s, mParameter)(PARAMETER_NAME_DATA(~, s, p))
#define PARAMETER_AS_CSTR(s, data, elem) Forte::FString(PARAMETER_NAME_DATA(~, s, p)).c_str()
#define PARAMETER_TYPES_AND_NAMES_REFERENCE(s, data, elem) const elem& PARAMETER_NAME_DATA(~, s, p)
#define PARAMETER_TYPES_AND_NAMES_DECLARE(s, data, elem) const elem PARAMETER_NAME_DATA(~, s, mParameter)
#define PARAMETER_VECTOR_PUSH_BACK(s, data, elem) result.push_back(Forte::FString(PARAMETER_NAME_DATA(~, s, mParameter)))


#define EXCEPTION_MAX_STACK_DEPTH 32
/*
 * Declare Parametric exceptions
 * DESC: format string: all substitution tokes must be of %s.
 * PARAM_TYPES must be sequences such as (Forte::FString)(int)
 *      types must have a FString Constructor
 */
#define EXCEPTION_PARAM_SUBCLASS(PARENT, NAME, DESC, PARAM_TYPES)                                       \
    class NAME : public PARENT                                                                          \
    {                                                                                                   \
    public:                                                                                             \
        inline NAME(BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(PARAMETER_TYPES_AND_NAMES_REFERENCE, 0, PARAM_TYPES))) : \
        mFormattedDescription(DESC),                                                                    \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(PARAMETER_INITIALIZE, 0, PARAM_TYPES))                 \
        {                                                                                               \
            mDescription.Format(DESC, BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(PARAMETER_AS_CSTR, 0, PARAM_TYPES))); \
        }                                                                                               \
        virtual ~NAME()  throw() {}                                                                     \
        virtual const std::string &GetFormattedDescription() const throw()                              \
        {                                                                                               \
            return mFormattedDescription;                                                               \
        }                                                                                               \
        virtual std::string GetFormattedDescriptionAsObjCStyle() const throw()                          \
        {                                                                                               \
            const boost::regex e("%[^%]");                                                              \
            return boost::regex_replace(mFormattedDescription, e, "%@");                                \
        }                                                                                               \
        virtual void GetParametersAsStrings(std::vector<std::string> &result) const throw()             \
        {                                                                                               \
            result.clear();                                                                             \
            BOOST_PP_SEQ_FOR_EACH(PARAMETER_APPEND_SEMICOLON, ~, BOOST_PP_SEQ_TRANSFORM(PARAMETER_VECTOR_PUSH_BACK, 0, PARAM_TYPES))         \
        }                                                                                               \
        Forte::FString mFormattedDescription;                                                           \
        BOOST_PP_SEQ_FOR_EACH(PARAMETER_APPEND_SEMICOLON, ~, BOOST_PP_SEQ_TRANSFORM(PARAMETER_TYPES_AND_NAMES_DECLARE, 0, PARAM_TYPES))      \
    };

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

        virtual const std::string &GetFormattedDescription() const throw()
        {
            return mDescription;
        }
        virtual std::string GetFormattedDescriptionAsObjCStyle() const throw()
        {
            return mDescription;
        }
        virtual void GetParametersAsStrings(std::vector<std::string> &result) const throw()
        {
            result.clear();
        }

        std::string ExtendedDescription();
        FString mDescription;

    protected:
        void getStack();
        void formatStack(Forte::FString &formattedStack);
        void * mStack[EXCEPTION_MAX_STACK_DEPTH];
        unsigned int mStackSize;
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

    EXCEPTION_PARAM_SUBCLASS(EFString, ParamException, "%s %s", (FString)(FString));
};
#endif
