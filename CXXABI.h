#ifndef __CXXABI_H__
#define __CXXABI_H__

#include <cxxabi.h>
#include <FString.h>

/*
 * Wrapper class for C++ ABI functions
 */
namespace Forte
{
    class CXXABI
    {
      public:
        static Forte::FString Demangle(const char *name)
        {
            int status = -4;
            char* res = abi::__cxa_demangle(name, NULL, NULL, &status);
            const char* const demangled_name = (status == 0) ? res : name;
            Forte::FString ret(demangled_name);
            if (res)
                free(res);

            return ret;
        }
    };
}

#endif
