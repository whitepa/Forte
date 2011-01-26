/* Copyright (c) 2008-2010 Scale Computing, Inc.  All Rights Reserved.
   Proprietary and Confidential */

#include "MockSSHRunner.h"

using namespace Forte;

int MockSSHRunner::Run(const FString &command,
                       FString *output,
                       FString *errorOutput)
{
    return mMockProcRunner.Run(command, 
                              "",
                              output,
                              PROC_RUNNER_NO_TIMEOUT);
}

void MockSSHRunner::ClearCommandList()
{
    mMockProcRunner.ClearCommandList();
}

StrList* MockSSHRunner::GetCommandList()
{
    return mMockProcRunner.GetCommandList();
}

void MockSSHRunner::ClearCommandResponseMap()
{
    mMockProcRunner.ClearCommandResponseMap();
}

void MockSSHRunner::SetCommandResponse(
    const FString& command, 
    const FString& response, 
    int response_code
    )
{
    mMockProcRunner.SetCommandResponse(
        command, response, response_code);
}

void MockSSHRunner::QueueCommandResponse(const FString& response, int response_code)
{
    mMockProcRunner.QueueCommandResponse(response, response_code);
}

bool MockSSHRunner::CommandWasRun(const FString& command)
{
    return mMockProcRunner.CommandWasRun(command);
}
