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
        ProcRunner() {}
        virtual ~ProcRunner() { }

        // API
        // timeout: -1 = default, 0 = none
        virtual int Run(const FString& command, 
                        const FString& cwd, 
                        FString *output,
                        unsigned int timeout, 
                        const StrStrMap *env = NULL,
                        const FString &infile = "/dev/null");

        virtual int RunNoTimeout(const FString& command, 
                                 const FString& cwd = "", 
                                 FString *output=NULL);

        virtual int RunBackground(const FString& command, 
                                  const FString& cwd = "",
                                  const StrStrMap *env = NULL,
                                  bool detach = true,
                                  int *pidReturn = NULL);

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
        
    protected:

    };

    typedef boost::shared_ptr<ProcRunner> ProcRunnerPtr;
};
#endif
