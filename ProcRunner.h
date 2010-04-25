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
        static ProcRunner* get();
        static ProcRunner& getRef();
        static ProcRunner* sSingleton;
        static Mutex sMutex;

    public:
        // API
        // timeout: -1 = default, 0 = none
        virtual int run(const FString& command, 
                        const FString& cwd, 
                        FString *output,
                        unsigned int timeout, 
                        const StrStrMap *env = NULL);

        virtual int run_background(const FString& command, 
                                   const FString& cwd = "",
                                   const StrStrMap *env = NULL);

        virtual FString read_pipe(const FString& command, int *exitval = NULL);

        virtual int rsh(const FString& ip, 
                        const FString& command, 
                        FString *output = NULL,
                        int timeout = -1);

        virtual int ssh(const FString& ip, 
                        const FString& command, 
                        FString *output = NULL,
                        int timeout = -1);

        virtual FString shell_escape(const FString& arg);
    };

// inline funcs for legacy code
    inline FString shell_escape(const FString& arg)
    {
        return ProcRunner::get()->shell_escape(arg);
    }

    inline FString read_pipe(const FString& command, int *exitval = NULL)
    {
        return ProcRunner::get()->read_pipe(command, exitval);
    }
};
#endif
