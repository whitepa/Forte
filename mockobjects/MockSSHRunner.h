/* Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
   Proprietary and Confidential */

#ifndef _MockSSHRunner_h
#define _MockSSHRunner_h

#include "SSHRunner.h"
#include "MockProcRunner.h"
#include "LogManager.h"

namespace Forte
{
    EXCEPTION_CLASS(EMockSSHRunner);

    class MockSSHRunner : public SSHRunner
    {
    public:
        virtual int Run(const FString &command,
                        FString *output,
                        FString *errorOutput);

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
        // we can use the mockprocrunner
        // to get the functionality of the 
        // mockSSHRunner
        MockProcRunner mMockProcRunner;
    };
}

#endif
