#ifndef _MockProcRunner_h
#define _MockProcRunner_h

#include "Forte.h"
#include "ProcRunner.h"

EXCEPTION_SUBCLASS(CException, EMockProcRunner);

class MockProcRunner : public ProcRunner
{
public:
    static void SetupSingleton();
    static void RealSingleton();

public:
    int run(const FString& command, 
            const FString& cwd, 
            FString *output,
            unsigned int timeout, 
            const StrStrMap *env = NULL);

    // mock specific
    void clearCommandList();
    StrList* getCommandList();

    void clearCommandResponseMap();

    void setCommandResponse(
        const FString& command, 
        const FString& response, 
        int response_code=0
        );

    void queueCommandResponse(const FString& response, int response_code=0);

    bool commandWasRun(const FString& command);

protected:
    StrList m_command_list;
    std::list<FString> m_response_queue;
    std::list<int> m_response_code_queue;
    StrStrMap m_command_response_map;
    StrIntMap m_command_response_code_map;

    static ProcRunner *s_real_pr;
    static MockProcRunner *s_mock_pr;
};


#endif
