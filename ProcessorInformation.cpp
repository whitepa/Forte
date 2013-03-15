#include "ProcessorInformation.h"
#include <boost/make_shared.hpp>

using namespace Forte;

ProcessorInformation::ProcessorInformation(ProcFileSystemPtr pfs) :
    mProcFileSystem(pfs)
{
    FTRACE;

    processInfo();
}

ProcessorInformation::~ProcessorInformation()
{
    FTRACE;
}

void ProcessorInformation::processInfo()
{
    FTRACE;

    std::vector<ProcFileSystem::CPUInfoPtr> processors;
    mProcFileSystem->GetProcessorInformation(processors);

    std::map<unsigned int, ProcessorSocketPtr>::const_iterator it;
    foreach (const ProcFileSystem::CPUInfoPtr& info, processors)
    {
        hlog(HLOG_DEBUG, "Found physical id: %u", info->mPhysicalId);
        ProcessorSocketPtr socket;
        if ((it = mSocketsById.find(info->mPhysicalId)) == mSocketsById.end())
        {
            socket = boost::make_shared<ProcessorSocket>(info->mPhysicalId);
            mSocketsById.insert(std::pair<unsigned int, ProcessorSocketPtr>(info->mPhysicalId, socket));
        }
        else
        {
            socket = (*it).second;
        }

        ProcessorPtr proc = boost::make_shared<Processor>(info);
        socket->AddProcessor(proc);
    }

}

unsigned int ProcessorInformation::GetNumberOfSockets() const
{
    FTRACE;

    return (mSocketsById.size());
}

unsigned int ProcessorInformation::GetTotalNumberOfCores() const
{
    FTRACE;

    unsigned int total = 0;
    std::map<unsigned int, ProcessorSocketPtr>::const_iterator it;
    for (it=mSocketsById.begin(); it != mSocketsById.end(); it++)
    {
        const ProcessorSocketPtr& socket = (*it).second;
        total += socket->GetNumberOfCores();
    }

    return total;
}

unsigned int ProcessorInformation::GetTotalNumberOfHardwareThreads() const
{
    FTRACE;

    unsigned int total = 0;
    std::map<unsigned int, ProcessorSocketPtr>::const_iterator it;
    for (it=mSocketsById.begin(); it != mSocketsById.end(); it++)
    {
        const ProcessorSocketPtr& socket = (*it).second;
        total += socket->GetNumberOfHardwareThreads();
    }

    return total;
}

unsigned int ProcessorInformation::GetNumberOfPhysicalCPUs() const
{
    FTRACE;

    return (mSocketsById.size());
}

double ProcessorInformation::GetClockFrequencyInHertz() const
{
    FTRACE;

    if (mSocketsById.size() == 0) return 0.0;

    std::map<unsigned int, ProcessorSocketPtr>::const_iterator it;
    it = mSocketsById.begin();

    return ((*it).second)->GetClockFrequencyInHertz();
}

bool ProcessorInformation::HasVirtualizationSupport() const
{
    FTRACE;

    if (mSocketsById.size() == 0) return false;

    std::map<unsigned int, ProcessorSocketPtr>::const_iterator it;
    for (it = mSocketsById.begin(); it != mSocketsById.end(); it++)
    {
        const ProcessorSocketPtr socket = (*it).second;
        if (socket->HasVirtualizationSupport())
        {
            return true;
        }
    }
    return false;
}
