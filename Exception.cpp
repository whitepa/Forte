// Exception.cpp
#include "Forte.h"
#include <sys/types.h>
#include <unistd.h>

using namespace Forte;

Exception::Exception()
{
    FTrace::GetStack(mStack);
}

Exception::Exception(const char *description) :
    mDescription(description)
{
    FTrace::GetStack(mStack);
}
Exception::Exception(const FStringFC &fc, const char *format, ...)
{
    va_list ap;
    int size;
    // get size
    va_start(ap, format);
    size = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    va_start(ap, format);
    mDescription.VFormat(format, size, ap);
    va_end(ap);

    FTrace::GetStack(mStack);
}
Exception::Exception(const Exception& other) :
    mDescription(other.mDescription),
    mStack(other.mStack)
{
}
Exception::~Exception() throw()
{
}

std::string Exception::ExtendedDescription()
{
    FString stack;
    FTrace::formatStack(mStack, stack);
    return FString(FStringFC(),"%s; STACK: %s", mDescription.c_str(), stack.c_str());
}
