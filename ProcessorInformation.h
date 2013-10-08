#ifndef __ProcessorInformation_h
#define __ProcessorInformation_h

#include "ProcFileSystem.h"
#include "FTrace.h"
#include <boost/make_shared.hpp>
#include "Foreach.h"

namespace Forte
{
    class Processor
    {
    public:
        Processor(const ProcFileSystem::CPUInfoPtr info) :
            mInfo(info),
            mClockFrequencyInHertz(info->mClockFrequencyInMegaHertz * 1000000) {}

        const ProcFileSystem::CPUInfoPtr mInfo;
        double mClockFrequencyInHertz;
    };
    typedef boost::shared_ptr<Processor> ProcessorPtr;

    class ProcessorSocket
    {
    public:
        ProcessorSocket(const unsigned int id) : mId(id),
                                                 mHyperthreadingEnabled(false) {
        }

        void AddProcessor(const ProcessorPtr proc) {
            FTRACE;
            const ProcFileSystem::CPUInfoPtr info = proc->mInfo;

            mProcessors.push_back(proc);
            std::set< unsigned int >::const_iterator it;
            if ((it = mCoresById.find(info->mCoreId)) == mCoresById.end())
            {
                hlog(HLOG_DEBUG, "Adding core %u", info->mCoreId);
                mCoresById.insert(info->mCoreId);
            }
            else
            {
                mHyperthreadingEnabled = true;
            }
        }

        unsigned int GetNumberOfCores() const {
            FTRACE;
            return mCoresById.size();
        }

        unsigned int GetNumberOfHardwareThreads() const {
            FTRACE;
            if (mProcessors.size() == 0) return 0;

            // @TODO: use number of siblings?
            unsigned int executionThreads;
            // mProcessors[0]->mInfo->HasHyperthreading()
            if (mHyperthreadingEnabled)
            {
                executionThreads = 2;
            }
            else
            {
                executionThreads = 1;
            }
            return (GetNumberOfCores() * executionThreads);
        }

        double GetClockFrequencyInHertz() const {
            FTRACE;
            if (mProcessors.empty()) return 0;
            return mProcessors[0]->mClockFrequencyInHertz;
        }

        bool HasVirtualizationSupport() const {
            foreach (const ProcessorPtr processor, mProcessors)
            {
                if (processor->mInfo->SupportsVirtualization())
                {
                    return true;
                }
            }
            return false;
        }
    protected:
        unsigned int mId;
        std::vector<ProcessorPtr> mProcessors;
        std::set<unsigned int > mCoresById;

        bool mHyperthreadingEnabled;
    };
    typedef boost::shared_ptr<ProcessorSocket> ProcessorSocketPtr;

    class ProcessorInformation : public Object
    {
    public:
        ProcessorInformation(ProcFileSystemPtr pfs);
        virtual ~ProcessorInformation();

        unsigned int GetNumberOfSockets() const;
        unsigned int GetTotalNumberOfCores() const;
        unsigned int GetTotalNumberOfHardwareThreads() const;
        unsigned int GetNumberOfPhysicalCPUs() const;
        double GetClockFrequencyInHertz() const;
        bool HasVirtualizationSupport() const;

    protected:
        ProcFileSystemPtr mProcFileSystem;
        std::map<unsigned int, ProcessorSocketPtr> mSocketsById;
        void processInfo();
    };
    typedef boost::shared_ptr<ProcessorInformation> ProcessorInformationPtr;

};
#endif
