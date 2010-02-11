#ifndef __Time_h_
#define __Time_h_

#ifndef FORTE_NO_BOOST
#ifdef FORTE_WITH_DATETIME

#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"

using namespace boost::gregorian; 
using namespace boost::local_time;
using namespace boost::posix_time;

#define TO_TIME_T(s, tz) CTime::f_to_time_t(__FILE__, __LINE__, # s, s, tz)

EXCEPTION_SUBCLASS(CForteException, CForteFTimeException);

class CTime {
public:
    static void init(const char *timezoneDatafile);

    // time_t <==> string conversions
    static time_t f_to_time_t(const char *filename, int line, const char *arg, const ptime &pt, const char *tz);
    static time_t f_to_time_t(const char *filename, int line, const char *arg, const ptime &pt, const std::string &tz);
    static time_t f_to_time_t(const char *filename, int line, const char *arg, const std::string &timeStr, const char *tz);
    static time_t f_to_time_t(const char *filename, int line, const char *arg, const std::string &timeStr, const std::string &tz);

    /// Convert a time to a string representation.
    ///
    static std::string to_str(const ptime &pt, const std::string &tz, const char *format = NULL);
    static std::string to_str(const time_t t, const std::string &tz, const char *format = NULL);
    static std::string to_str(const ptime &pt, const char *tz, const char *format = NULL);
    static std::string to_str(const time_t t, const char *tz, const char *format = NULL);

    // replacement for boost's broken from_time_t()
    static ptime from_time_t(time_t t, const char *tz);

    // get the day
    static time_t day(time_t t, const char *tz);
    static time_t day(const ptime pt, const char *tz);

protected:
    static CMutex sLock;
    static tz_database sTimezoneDb;
    static bool sInitialized;
};

#endif // FORTE_WITH_DATETIME
#endif // FORTE_NO_BOOST

#endif // __Time_h_
