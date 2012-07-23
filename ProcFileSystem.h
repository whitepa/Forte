#ifndef __forte_ProcFileSystem_h
#define __forte_ProcFileSystem_h

#include "Exception.h"
#include "FileSystem.h"
#include "Context.h"
#include "Types.h"
#include <boost/shared_ptr.hpp>
#include "FTrace.h"

namespace Forte
{
    // typed exceptions
    EXCEPTION_SUBCLASS(Exception, EProcFileSystem);
    EXCEPTION_SUBCLASS2(EProcFileSystem, EProcFileSystemProcessNotFound,
                        "Could not find pid for requested process");

    /**
     * \class ProcFileSystem
     *
     * This class will be used to read and parse various info from the
     * /proc virtual filesystem on linux, throwing exceptions as
     * appropriate and returned typed data to the caller
     **/

    class ProcFileSystem : public Object
    {
    public:
        ProcFileSystem(const Forte::FileSystemPtr &fsPtr) :
            mFileSystemPtr(fsPtr) {}
        virtual ~ProcFileSystem() {};

        /**
         * \class Uptime information on the amount of time a system
         * has been up
         **/
        class Uptime
        {
        public:
            double mSecondsUp;     ///< Seconds since last reboot
            double mSecondsIdle;   ///< Amount of time spent idle
        };

        /**
         * takes an Uptime class, fills in the details from /proc/uptime
         **/
        virtual void UptimeRead(Uptime& uptime);

        /**
         * takes a map of string to double. fills in the details from
         * /proc/meminfo as name => double pairs
         **/
        virtual void MemoryInfoRead(Forte::StrDoubleMap& meminfo);

        virtual unsigned int CountOpenFileDescriptors(pid_t pid = getpid() );

        virtual void PidOf(const FString& runningProg, std::vector<pid_t>& pids) const;
        virtual bool ProcessIsRunning(const FString& runningProg,
                                      const FString& pidFile = "") const;
        virtual void SetOOMScore(pid_t pid, const FString &score);


        /**
         * \class CPUInfo information about a cpu
         */
        class CPUInfo
        {
        public:
            unsigned int mProcessorNumber;
            Forte::FString mVendorId;
            unsigned int mCPUFamily;
            unsigned int mModel;
            Forte::FString mModelName;
            unsigned int mStepping;
            double mClockFrequencyInMegaHertz;
            unsigned int mPhysicalId;
            unsigned int mCoreId;
            unsigned int mNumberOfCores;
            bool mHasFPU;
            bool mHasFPUException;
            unsigned int mSiblings;

            void AddFlags(const std::vector<FString>& flags) {
                foreach (const FString& flag, flags)
                {
                    hlog(HLOG_DEBUG, "Adding flag '%s'", flag.c_str());
                    mFlagsSet.insert(flag);
                }
            }

            bool HasFlag(const FString& check) const {
                FTRACE2("%s (%zu)", check.c_str(), mFlagsSet.size());
                return (mFlagsSet.find(check) != mFlagsSet.end());
            }

            bool SupportsVirtualization() const {
                return (HasFlag("vmx") || HasFlag("svm"));
            }

            bool HasHyperthreading() const {
                return (HasFlag("ht"));
            }
        protected:
            std::set<FString> mFlagsSet;
        };
        typedef boost::shared_ptr<CPUInfo> CPUInfoPtr;

        void GetProcessorInformation(std::vector<CPUInfoPtr>& processors) const;
    protected:
        void getProcFileContents(const FString& pathInSlashProc,
                                 FString& contents);


        Forte::FileSystemPtr mFileSystemPtr;
    };

    typedef boost::shared_ptr<ProcFileSystem> ProcFileSystemPtr;
};
#endif
