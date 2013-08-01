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
        /**
         *Runs a command in the foreground and returns its resulting output.
         *@param command is the command you want to run.
         *@param cwd is the current working directory the command should be run in.
         *Pass an empty string to use the default.
         *@param [out] output returns stdout from command. Pass NULL if you do not want anything returned back.
         *@param timeout is the amount of time to let the command run before timing out. Pass 0 for no time out.
         *@param env lets you pass in specific environment. Use NULL for no environment specifics.
         *@param infile lets you redirect standard in. Use /dev/null for no redirect.
         **/

        virtual int Run(const FString& command,
                        const FString& cwd,
                        FString *output,
                        unsigned int timeout,
                        const StrStrMap *env = NULL,
                        const FString &infile = "/dev/null");

        virtual int RunNoTimeout(const FString& command,
                                 const FString& cwd = "",
                                 FString *output=NULL);

        /**
         *Runs a command in the background.
         *@param command is the command you want to run.
         *@param cwd is the current working directory the command should be run in.
         *Pass an empty string to use the default.
         *@param env lets you pass in specific environment. Use NULL for no environment specifics.
         *@param detach detaches from the parent process and runs as a daemon when true.
         *@param pidReturn returns the process identifier (PID) for the process created.
         *Use NULL if you don’t want this returned. Detach must be false if you want the PID returned.
         **/
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

        virtual FString ShellEscape(const FString& arg);

    protected:

    };

    typedef boost::shared_ptr<ProcRunner> ProcRunnerPtr;
};
#endif
