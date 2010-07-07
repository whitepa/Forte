#ifndef __ProcessManager_h
#define __ProcessManager_h

#include "Types.h"
#include "Object.h"
#include "Thread.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <sys/types.h>

namespace Forte
{


    class ProcessHandler;

    class ProcessManager : public Thread
    {
    public:
		typedef std::map<FString, boost::shared_ptr<ProcessHandler> > ProcessHandlerMap;
		typedef std::map<pid_t, boost::shared_ptr<ProcessHandler> > RunningProcessHandlerMap;
		typedef std::pair<pid_t, boost::shared_ptr<ProcessHandler> > ProcessHandlerPair;
		
        ProcessManager();
        virtual ~ProcessManager();

        virtual boost::shared_ptr<ProcessHandler> CreateProcess(const FString &command,
                                                                const FString &currentWorkingDirectory = "/",
                                                                const FString &inputFilename = "/dev/null",
																const FString &outputFilename = "/dev/null",
																const FString &errorFilename = "/dev/null",
                                                                const StrStrMap *environment = NULL);
		virtual void RunProcess(const FString &guid);
		virtual void AbandonProcess(const FString &guid);
		
    private:
		virtual void * run(void);
		
		ProcessHandlerMap processHandlers;
        RunningProcessHandlerMap runningProcessHandlers;
		Mutex mLock;
    };

};
#endif
