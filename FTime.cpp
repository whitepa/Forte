#include "Forte.h"
#ifndef FORTE_NO_BOOST
#ifdef FORTE_WITH_DATETIME
#include "FTime.h"


Mutex FTime::sLock;
tz_database FTime::sTimezoneDb;
bool FTime::sInitialized = false;

void FTime::init(const char *timezoneDatafile)
{
    try
    {
        AutoUnlockMutex lock(sLock); 
        sTimezoneDb.load_from_file(timezoneDatafile);
        sInitialized = true;
    }
    catch(data_not_accessible &dna)
    {
        hlog(HLOG_ERR, "Error with time zone data file: %s", dna.What());
    }
    catch(bad_field_count &bfc)
    {
        hlog(HLOG_ERR, "Bad field count error with time zone data file: %s", bfc.What());
    }
}
ptime FTime::from_time_t(time_t t, const char *tz)
{
    AutoUnlockMutex lock(sLock);
    if (!sInitialized)
        throw ForteUninitializedException("FTime class has not been initialized");
    time_zone_ptr tzp = sTimezoneDb.time_zone_from_region(tz);
    time_zone_ptr utc_tzp = sTimezoneDb.time_zone_from_region("UTC");
    ptime epoch(date(1970, 1, 1));
//    cout << "from_time_t: " << t << "  (TZ " << tz << ")" << endl;
//    cout << "Epoch: " << epoch << endl;
    local_date_time ldt(epoch + seconds(t), tzp);
//    cout << "LDT: " << ldt << endl;
    ptime ret = ldt.local_time();
//    cout << "Ret: " << ret << endl;
    return ret;
}
time_t FTime::day(time_t t, const char *tz)
{
    ptime pt = FTime::from_time_t(t, tz);
    return day(pt, tz);
}
time_t FTime::day(const ptime pt, const char *tz)
{
    AutoUnlockMutex lock(sLock);
    if (!sInitialized)
        throw ForteUninitializedException("FTime class has not been initialized");
    try
    {
        time_zone_ptr tzp = sTimezoneDb.time_zone_from_region(tz);
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
        hlog(HLOG_ERR, "FTime::to_time_t(): %s", e.What());
        throw e;
    }

}
time_t FTime::f_to_time_t(const char *filename, int line, const char * arg, const ptime &pt, const char *tz)
{
    AutoUnlockMutex lock(sLock);
    if (!sInitialized)
        throw ForteUninitializedException("FTime class has not been initialized");
    try
    {
        time_zone_ptr tzp = sTimezoneDb.time_zone_from_region(tz);
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
             filename, line, arg, e.What());
        throw ForteFTimeException(FStringFC(), "Invalid time in argument '%s' (%s:%d): %s",
                         arg, filename, line, e.What());
    }
    catch (...)
    {
        hlog(HLOG_ERR, "FTime::f_to_time_t(): called from %s:%d (argument %s); unknown exception", 
             filename, line, arg);
        throw ForteFTimeException(FStringFC(), "Invalid time in argument '%s' (%s:%d); unknown exception",
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
             filename, line, arg, e.What());
        throw ForteFTimeException(FStringFC(), "Invalid time in argument '%s' (%s:%d): %s",
                         arg, filename, line, e.What());
    }
    catch (...)
    {
        hlog(HLOG_ERR, "FTime::f_to_time_t(): called from %s:%d (argument %s); unknown exception", 
             filename, line, arg);
        throw ForteFTimeException(FStringFC(), "Invalid time in argument '%s' (%s:%d); unknown exception",
                         arg, filename, line);
    }
    return f_to_time_t(filename, line, arg, pt, tz);
}
time_t FTime::f_to_time_t(const char *filename, int line, const char * arg, const std::string &timeStr, const std::string &tz)
{
    return f_to_time_t(filename, line, arg, timeStr, tz.c_str());
}

std::string FTime::to_str(const ptime &pt, const char *tz, const char *format)
{
    AutoUnlockMutex lock(sLock);
    if (!sInitialized)
        throw ForteUninitializedException("FTime class has not been initialized");
    time_zone_ptr tzp = sTimezoneDb.time_zone_from_region(tz);
//    time_zone_ptr utc_tzp = sTimezoneDb.time_zone_from_region("UTC");
//    local_date_time utc(pt, utc_tzp);
//    local_date_time zoneTime = utc.local_time_in(tzp);
    local_date_time zoneTime(pt, tzp);
//    cout << "to_str: ptime " << pt << " (TZ " << tz << ")" << endl;
    stringstream ss;
    local_time_facet *output_facet = new local_time_facet();
    ss.imbue(locale(locale::classic(), output_facet));
    if (format == NULL)
        output_facet->format("%Y-%m-%d %T");
    else
        output_facet->format(format);
    ss << zoneTime;
//    cout << "str = " << ss.str() << endl;
    return ss.str();
}
std::string FTime::to_str(const ptime &pt, const std::string &tz, const char *format)
{
    return to_str(pt, tz.c_str(), format);
}
std::string FTime::to_str(const time_t t, const char *tz, const char *format)
{
    ptime pt = FTime::from_time_t(t, "UTC");
    return to_str(pt, tz, format);
}

std::string FTime::to_str(const time_t t, const std::string &tz, const char *format)
{
    return to_str(t, tz.c_str(), format);
}

#endif
#endif // FORTE_NO_BOOST
