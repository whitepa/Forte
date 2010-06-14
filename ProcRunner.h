#ifndef _ProcRunner_h
#define _ProcRunner_h

#include "Types.h"
#include "AutoMutex.h"
#include "Object.h"

//TODO: better shorter constants
#define PROC_RUNNER_NO_TIMEOUT        0
#define PROC_RUNNER_DEFAULT_TIMEOUT 120
#define PROC_RUNNER_LONG_TIMEOUT    300


namespace Forte
{
    class ProcRunner : public Object
    {
    public:
        // ctor/dtor
        virtual ~ProcRunner() { }
        void DeleteSingleton();

        // singleton
        static ProcRunner* Get();
        static ProcRunner& GetRef();
        static ProcRunner* sSingleton;
        static Mutex sMutex;

    public:
        // API
        // timeout: -1 = default, 0 = none
        virtual int Run(const FString& command, 
                        const FString& cwd, 
                        FString *output,
                        unsigned int timeout, 
                        const StrStrMap *env = NULL,
                        const FString &infile = "/dev/null");

        virtual int RunBackground(const FString& command, 
                                   const FString& cwd = "",
                                   const StrStrMap *env = NULL);

        virtual FString ReadPipe(const FString& command, int *exitval = NULL);

        virtual int Rsh(const FString& ip, 
                        const FString& command, 
                        FString *output = NULL,
                        int timeout = -1);

        virtual int Ssh(const FString& ip, 
                        const FString& command, 
                        FString *output = NULL,
                        int timeout = -1);

        virtual FString ShellEscape(const FString& arg);
    };

    inline FString ShellEscape(const FString& arg)
    {
        return ProcRunner::Get()->ShellEscape(arg);
    }

    inline FString ReadPipe(const FString& command, int *exitval = NULL)
    {
        return ProcRunner::Get()->ReadPipe(command, exitval);
    }

};
#endif
