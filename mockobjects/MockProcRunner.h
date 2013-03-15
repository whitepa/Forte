#ifndef _MockProcRunner_h
#define _MockProcRunner_h

#include "ProcRunner.h"
#include "Exception.h"
#include "LogManager.h"

namespace Forte
{
    EXCEPTION_CLASS(EMockProcRunner);
    EXCEPTION_SUBCLASS(EMockProcRunner, EUnexpectedCommand);

    struct ExpectedCommandResponse
    {
        FString mCommand;
        FString mResponse;
        int mResponseCode;
    };

    class MockProcRunner : public ProcRunner
    {
    public:
        int Run(const FString& command,
                const FString& cwd,
                FString *output,
                unsigned int timeout,
                const StrStrMap *env = NULL,
                const FString &infile = "/dev/null");

        // mock specific
        void ClearCommandList();
        StrList* GetCommandList();

        void ClearCommandResponseMap();

        void SetCommandResponse(
            const FString& command,
            const FString& response,
            int response_code=0
            );

        void QueueCommandResponse(const FString& response, int response_code=0);
        void QueueCommandResponse(const FString& expectedCommand,
                                  const FString& response, int response_code=0);

        void ClearCommandResponseQueue();

        bool CommandWasRun(const FString& command);

    protected:
        StrList m_command_list;
        std::list<ExpectedCommandResponse> m_response_queue;

        StrStrMap m_command_response_map;
        StrIntMap m_command_response_code_map;

    };
}
#endif
