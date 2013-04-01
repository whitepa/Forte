// Exception.cpp
#include "Exception.h"
#include "FTrace.h"
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>

using namespace Forte;

Exception::Exception()
{
    getStack();
}

Exception::Exception(const char *description) :
    mDescription(description)
{
    getStack();
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

    getStack();
}
Exception::Exception(const Exception& other) :
    mDescription(other.mDescription)
{
    mStackSize = other.mStackSize;
    for (unsigned i = 0; i < mStackSize; i++)
        mStack[i] = other.mStack[i];
}
Exception::~Exception() throw()
{
}

void Exception::getStack()
{
    mStackSize = backtrace(mStack, EXCEPTION_MAX_STACK_DEPTH);
}

void Exception::formatStack(Forte::FString &formattedStack)
{
    formattedStack.clear();
    char **stack = backtrace_symbols(mStack, mStackSize);
    if (!stack)
        return;

    std::vector<Forte::FString> traceVector;
    for(unsigned i = 0; i < mStackSize; i++)
    {
        traceVector.push_back(Forte::FString(stack[i]));
    }
    free(stack);

    formattedStack.Implode("\n", traceVector);
}

std::string Exception::ExtendedDescription()
{
    FString stack;
    formatStack(stack);
    return FString(FStringFC(),"%s; STACK:\n%s",
            mDescription.c_str(), stack.c_str());
}
