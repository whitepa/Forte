#ifndef __ProcessHandler_h
#define __ProcessHandler_h

#include "Types.h"
#include "Object.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>


namespace Forte
{

    /**
     * A handle to a process
     *
     */
    class ProcessHandler : public Object 
    {
    public:
	typedef boost::function<void (boost::shared_ptr<ProcessHandler>)> ProcessCompleteCallback;


	/**
	 * Construct a ProcessHandle object. Instantiating the object does not
	 * automatically cause the command to run. You must call Run to kick off
	 * the execution.
	 * @param command The command you want to run
	 * @param processCompleteCallback The callback to call when the process has completed
	 * @param currentWorkingDirectory The directory that should be used as the current working
	 * directory for the forked process.
	 * @param environment a map of the environment to apply to the command
	 * @param inputFilename name of the file to use for input.
	 */
	ProcessHandler(const FString &command,
		      ProcessCompleteCallback processCompleteCallback,
		      const FString &currentWorkingDirectory = "/",
		      const StrStrMap *environment = NULL,
		      const FString &inputFilename = "/dev/null");
	virtual ~ProcessHandler();
	
	void SetCurrentWorkingDirectory(const FString &cwd);
	void SetTimeout(unsigned int timeout);
	void SetEnvironment(const StrStrMap *env);
	void SetInputFilename(const FString &infile);
	void SetLoiter(bool loiter);
	
	void Run(unsigned int timeout);
	void Cancel();
	bool IsRunning();
	unsigned int GetStatusCode();
	const FString& GetOutputString();
	
    protected:
	
	FString mCommand;
	ProcessCompleteCallback mProcessCompleteCallback;
	FString mCurrentWorkingDirectory;
	StrStrMap mEnvironment;
	FString mInputFilename;

	unsigned int mTimeout;
	bool mShouldLoiter;
	bool mIsRunning;
	
	virtual FString shellEscape(const FString& arg);
    };


};
#endif
