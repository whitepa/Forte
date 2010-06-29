#ifndef _MockProcRunner_h
#define _MockProcRunner_h

#include "Forte.h"
#include "ProcRunner.h"
#include "Exception.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, EMockProcRunner);

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

        bool CommandWasRun(const FString& command);

    protected:
        StrList m_command_list;
        std::list<FString> m_response_queue;
        std::list<int> m_response_code_queue;
        StrStrMap m_command_response_map;
        StrIntMap m_command_response_code_map;

    };
}
#endif
