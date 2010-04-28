#ifndef __forte_ProcFileSystem_h
#define __forte_ProcFileSystem_h

#include "Exception.h"
#include "Context.h"
#include <boost/shared_ptr.hpp>

namespace Forte
{
    // typed exceptions
    EXCEPTION_SUBCLASS(Exception, EProcFileSystem);

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
        ProcFileSystem(Context& ctxt);
        virtual ~ProcFileSystem();

        /**
         * \class Uptime information on the amount of time a system
         * has been up
         **/
        class Uptime
        {
        public:
            double secondsUp;     ///< Seconds since last reboot
            double secondsIdle;   ///< Amount of time spent idle
        };

        /**
         * takes an Uptime class, fills in the details from /proc/uptime
         **/
        virtual void UptimeRead(Uptime& uptime);


    protected:
        Context &mContext;
    };

    typedef boost::shared_ptr<ProcFileSystem> ProcFileSystemPtr;
};
#endif
