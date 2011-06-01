#ifndef __Time_h_
#define __Time_h_

#include "Exception.h"
#include "AutoMutex.h"
#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"

using namespace boost::gregorian; 
using namespace boost::local_time;
using namespace boost::posix_time;

#define TO_TIME_T(s, tz) FTime::f_to_time_t(__FILE__, __LINE__, # s, s, tz)

EXCEPTION_CLASS(EFTime);

namespace Forte
{
    class FTime : public Forte::Object
    {
    public:
        FTime(const char *timezoneDatafile);
        virtual ~FTime() {};

        // time_t <==> string conversions
        time_t f_to_time_t(const char *filename, int line, const char *arg, const ptime &pt, const char *tz);
        time_t f_to_time_t(const char *filename, int line, const char *arg, const ptime &pt, const std::string &tz);
        time_t f_to_time_t(const char *filename, int line, const char *arg, const std::string &timeStr, const char *tz);
        time_t f_to_time_t(const char *filename, int line, const char *arg, const std::string &timeStr, const std::string &tz);

        /// Convert a time to a string representation.
        ///
        std::string ToStr(const ptime &pt, const std::string &tz, const char *format = NULL);
        std::string ToStr(const time_t t, const std::string &tz, const char *format = NULL);
        std::string ToStr(const ptime &pt, const char *tz, const char *format = NULL);
        std::string ToStr(const time_t t, const char *tz, const char *format = NULL);

        // replacement for boost's broken FromTimeT()
        ptime FromTimeT(time_t t, const char *tz);

        // get the day
        time_t Day(time_t t, const char *tz);
        time_t Day(const ptime pt, const char *tz);
    protected:
        Forte::Mutex mLock;
        tz_database mTimezoneDb;
    };
};

#endif // __Time_h_
