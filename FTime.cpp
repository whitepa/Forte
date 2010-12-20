#include "FTime.h"
#include "LogManager.h"

using namespace std;
using namespace Forte;

FTime::FTime(const char *timezoneDatafile)
{
    try
    {
        AutoUnlockMutex lock(mLock);
        mTimezoneDb.load_from_file(timezoneDatafile);
    }
    catch(data_not_accessible &dna)
    {
        hlog(HLOG_ERR, "Error with time zone data file: %s", dna.what());
    }
    catch(bad_field_count &bfc)
    {
        hlog(HLOG_ERR, "Bad field count error with time zone data file: %s", bfc.what());
    }
}
ptime FTime::FromTimeT(time_t t, const char *tz)
{
    AutoUnlockMutex lock(mLock);
    time_zone_ptr tzp = mTimezoneDb.time_zone_from_region(tz);
    time_zone_ptr utc_tzp = mTimezoneDb.time_zone_from_region("UTC");
    ptime epoch(date(1970, 1, 1));
//    cout << "from_time_t: " << t << "  (TZ " << tz << ")" << endl;
//    cout << "Epoch: " << epoch << endl;
    local_date_time ldt(epoch + seconds(t), tzp);
//    cout << "LDT: " << ldt << endl;
    ptime ret = ldt.local_time();
//    cout << "Ret: " << ret << endl;
    return ret;
}
time_t FTime::Day(time_t t, const char *tz)
{
    ptime pt = FTime::FromTimeT(t, tz);
    return Day(pt, tz);
}
time_t FTime::Day(const ptime pt, const char *tz)
{
    AutoUnlockMutex lock(mLock);
    try
    {
        time_zone_ptr tzp = mTimezoneDb.time_zone_from_region(tz);
        date in_date(pt.date());
//        cout << "in_date: " << in_date << endl;
        time_duration td; // use 00:00
//        cout << "td: " << td << endl;
        local_date_time ldt(in_date, td, tzp, local_date_time::NOT_DATE_TIME_ON_ERROR);
//        cout << "LOCAL DATETIME: " << ldt << endl;
        ptime epoch(date(1970, 1, 1));
//        cout << epoch << endl;
        time_duration dur = ldt.utc_time() - epoch;
//        cout << dur << endl;
//        cout << "Day as time_t: " << dur.total_seconds() << endl;
        return dur.total_seconds();
    }
    catch (std::exception &e)
    {
        hlog(HLOG_ERR, "FTime::to_time_t(): %s", e.what());
        throw e;
    }

}
time_t FTime::f_to_time_t(const char *filename, int line, const char * arg, const ptime &pt, const char *tz)
{
    AutoUnlockMutex lock(mLock);
    try
    {
        time_zone_ptr tzp = mTimezoneDb.time_zone_from_region(tz);
        date in_date(pt.date());
//    cout << "in_date: " << in_date << endl;
        time_duration td(pt.time_of_day());
//    cout << "td: " << td << endl;
        local_date_time ldt(in_date, td, tzp, local_date_time::NOT_DATE_TIME_ON_ERROR);
//    cout << "LOCAL DATETIME: " << ldt << endl;
        ptime epoch(date(1970, 1, 1));
//    cout << epoch << endl;
        time_duration dur = ldt.utc_time() - epoch;
//    cout << dur << endl;
        return dur.total_seconds();
    }
    catch (std::exception &e)
    {
        hlog(HLOG_ERR, "FTime::f_to_time_t(): called from %s:%d (argument %s): %s", 
             filename, line, arg, e.what());
        throw EFTime(FStringFC(), "Invalid time in argument '%s' (%s:%d): %s",
                     arg, filename, line, e.what());
    }
    catch (...)
    {
        hlog(HLOG_ERR, "FTime::f_to_time_t(): called from %s:%d (argument %s); unknown exception", 
             filename, line, arg);
        throw EFTime(FStringFC(), "Invalid time in argument '%s' (%s:%d); unknown exception",
                     arg, filename, line);
    }
}
time_t FTime::f_to_time_t(const char *filename, int line, const char * arg, const ptime &pt, const std::string &tz)
{
    return f_to_time_t(filename, line, arg, pt, tz.c_str());
}

time_t FTime::f_to_time_t(const char *filename, int line, const char * arg, const std::string &timeStr, const char *tz)
{
    ptime pt;
    try 
    {
        pt = time_from_string(timeStr);
    }
    catch (std::exception &e)
    {
        hlog(HLOG_ERR, "FTime::f_to_time_t(): called from %s:%d (argument %s): %s", 
             filename, line, arg, e.what());
        throw EFTime(FStringFC(), "Invalid time in argument '%s' (%s:%d): %s",
                     arg, filename, line, e.what());
    }
    catch (...)
    {
        hlog(HLOG_ERR, "FTime::f_to_time_t(): called from %s:%d (argument %s); unknown exception", 
             filename, line, arg);
        throw EFTime(FStringFC(), "Invalid time in argument '%s' (%s:%d); unknown exception",
                     arg, filename, line);
    }
    return f_to_time_t(filename, line, arg, pt, tz);
}
time_t FTime::f_to_time_t(const char *filename, int line, const char * arg, const std::string &timeStr, const std::string &tz)
{
    return f_to_time_t(filename, line, arg, timeStr, tz.c_str());
}

std::string FTime::ToStr(const ptime &pt, const char *tz, const char *format)
{
    AutoUnlockMutex lock(mLock);
    time_zone_ptr tzp = mTimezoneDb.time_zone_from_region(tz);
    local_date_time zoneTime(pt, tzp);
    stringstream ss;
    local_time_facet *output_facet = new local_time_facet();
    ss.imbue(locale(locale::classic(), output_facet));
    if (format == NULL)
        output_facet->format("%Y-%m-%d %T");
    else
        output_facet->format(format);
    ss << zoneTime;
    return ss.str();
}
std::string FTime::ToStr(const ptime &pt, const std::string &tz, const char *format)
{
    return ToStr(pt, tz.c_str(), format);
}
std::string FTime::ToStr(const time_t t, const char *tz, const char *format)
{
    ptime pt = FTime::FromTimeT(t, "UTC");
    return ToStr(pt, tz, format);
}

std::string FTime::ToStr(const time_t t, const std::string &tz, const char *format)
{
    return ToStr(t, tz.c_str(), format);
}

