#include "FString.h"
#include "Foreach.h"
#include "LogManager.h"
#include "SystemCallUtil.h"
#include <cstdio>
#include <cstdarg>
#include <errno.h>
#include <boost/regex.hpp>

using namespace Forte;

FString::FString()
{
}

FString::FString(const FStringFC &f, const char *format, ...)
{
    va_list ap;
    int size;
    // get size
    va_start(ap, format);
    size = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    va_start(ap, format);
    VFormat(format, size, ap);
    va_end(ap);
}

FString::FString(const std::vector<unsigned int> &intvec)
{
    bool first = true;
    std::vector<unsigned int>::const_iterator i;
    for (i = intvec.begin(); i != intvec.end(); ++i)
    {
        if (!first) append(",");
        append(FString(*i));
        first = false;
    }
}
FString::FString(const std::vector<boost::optional<uint32_t> > &intvec)
{
    bool first = true;
    std::vector<boost::optional<uint32_t> >::const_iterator i;
    for (i = intvec.begin(); i != intvec.end(); ++i)
    {
        if (!first) append(",");
        if (*i)
            append(FString(i->get()));
        else
            append("_");
        first = false;
    }
}
FString::FString(const std::set<unsigned int> &intset)
{
    bool first = true;
    std::set<unsigned int>::const_iterator i;
    for (i = intset.begin(); i != intset.end(); ++i)
    {
        if (!first) append(",");
        append(FString(*i));
        first = false;
    }
}
FString::FString(const std::set<std::string> &strset, bool quotes)
{
    bool first = true;
    std::set<std::string>::const_iterator i;
    for (i = strset.begin(); i != strset.end(); ++i)
    {
        if (!first) append(",");
        if (quotes) append("'");
        append(FString(*i));
        if (quotes) append("'");
        first = false;
    }
}
FString::FString(const std::set<FString> &strset, bool quotes)
{
    bool first = true;
    std::set<FString>::const_iterator i;
    for (i = strset.begin(); i != strset.end(); ++i)
    {
        if (!first) append(",");
        if (quotes) append("'");
        append(*i);
        if (quotes) append("'");
        first = false;
    }
}
FString::FString(const std::vector<int> &intvec)
{
    bool first = true;
    std::vector<int>::const_iterator i;
    for (i = intvec.begin(); i != intvec.end(); ++i)
    {
        if (!first) append(",");
        append(FString(*i));
        first = false;
    }
}
FString::FString(const std::set<int> &intset)
{
    bool first = true;
    std::set<int>::const_iterator i;
    for (i = intset.begin(); i != intset.end(); ++i)
    {
        if (!first) append(",");
        append(FString(*i));
        first = false;
    }
}

FString::FString(const struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        char buf[INET_ADDRSTRLEN];
        if (!inet_ntop(sa->sa_family, &((reinterpret_cast<struct sockaddr_in *>(const_cast<struct sockaddr*>(sa)))->sin_addr),
                       buf, INET_ADDRSTRLEN))
            throw EFString(SystemCallUtil::GetErrorDescription(errno));
        assign(buf);
    }
    else if (sa->sa_family == AF_INET6)
    {
        char buf[INET6_ADDRSTRLEN];
        if (!inet_ntop(sa->sa_family, &((reinterpret_cast<struct sockaddr_in *>(const_cast<struct sockaddr*>(sa)))->sin_addr),
                       buf, INET6_ADDRSTRLEN))
            throw EFString(SystemCallUtil::GetErrorDescription(errno));
        assign(buf);
    }
    else
        throw EFStringUnknownAddressFamily(FStringFC(), "AF %d not supported", sa->sa_family);
}

FString::FString(const FStringFO &f, const FString &format, const std::vector<FString> &parameters)
{
    OFormat(format, parameters);
}

FString::FString(const FStringFO &f, const FString &format, const std::vector<std::string> &parameters)
{
    OFormat(format, parameters);
}

void FString::Format(const char *format, ...)
{
    va_list ap;
    int size;

    // get size
    va_start(ap, format);
    size = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    // format string
    clear();
    reserve(size + 1);
    resize(size);
    va_start(ap, format);
    vsnprintf(const_cast<char*>(data()), size + 1, format, ap);
    va_end(ap);
}
void FString::VFormat(const char *format, int size, va_list ap)
{
    clear();
    reserve(size + 1);
    resize(size);
    vsnprintf(const_cast<char*>(data()), size + 1, format, ap);
}

FString& FString::Replace(const char* str_find, const char* str_replace)
{
    return Replace(0, str_find, str_replace);
}

FString& FString::Replace(std::string::size_type start, const char* str_find, const char* str_replace)
{
    std::string::size_type  pos;
    std::string::size_type  len_f = strlen(str_find), len_r = strlen(str_replace);

    while ((pos = find(str_find, start)) != NOPOS)
    {
        replace(pos, len_f, str_replace);
        start = pos + len_r;
    }

    return *this;
}

FString& FString::StripNonMatchingChars(const char* whitelist)
{
    std::string::size_type pos = 0;

    while ((pos = find_first_not_of(whitelist, pos)) != std::string::npos)
    {
        erase(pos, 1);
    }

    return *this;
}

FString& FString::TrimLeft(const char* strip_chars)
{
    std::string::size_type trim;
    trim = find_first_not_of(strip_chars);
    erase(0, trim);
    return *this;
}


FString& FString::TrimRight(const char* strip_chars)
{
    std::string::size_type trim;
    trim = find_last_not_of(strip_chars);
    erase(trim + 1);
    return *this;
}

// a lot of symmetry in Chop/String/Right/Left below
FString& FString::ChopRight(const char* delimiters)
{
    if (! IsDelimited(delimiters))
        return *this;
    std::string::size_type trim;
    trim = find_last_of(delimiters);
    erase(trim);
    return *this;
}

FString& FString::ChopLeft(const char* delimiters)
{
    if (! IsDelimited(delimiters))
        return *this;
    std::string::size_type trim;
    trim = find_first_of(delimiters);
    erase(0, trim + 1);
    return *this;
}

FString& FString::RightString(const char* delimiters)
{
    if (! IsDelimited(delimiters))
        return *this;
    std::string::size_type trim;
    trim = find_last_of(delimiters);
    erase(0, trim + 1);
    return *this;
}

FString& FString::LeftString(const char* delimiters)
{
    if (! IsDelimited(delimiters))
        return *this;
    std::string::size_type trim;
    trim = find_first_of(delimiters);
    erase(trim);
    return *this;
}

bool FString::IsDelimited(const char* delimiters)
{
    std::string::size_type location;
    location = find_last_of(delimiters);
    if (location == npos)
        return false;
    return true;
}

int FString::NumDelimiters(const char* delimiters)
{
    int num;
    std::string s = *this;
    num = std::count(s.begin(), s.end(), delimiters[0]);
    return num;
}


FString& FString::MakeUpper()
{
    int i, n = length();
    char d = 'a' - 'A';

    for (i=0; i<n; i++)
    {
        char &c = operator [](i);

        if (c >= 'a' && c <= 'z')
        {
            c = c - d;
        }
    }

    return *this;
}


FString& FString::MakeLower()
{
    int i, n = length();
    char d = 'a' - 'A';

    for (i=0; i<n; i++)
    {
        char &c = operator [](i);

        if (c >= 'A' && c <= 'Z')
        {
            c = c + d;
        }
    }

    return *this;
}

bool FString::RegexMatch(const char *exp, std::vector<FString> &matching_strings) const
{
    matching_strings.clear();
    boost::regex expression(exp);
    boost::cmatch matches;
    if (! boost::regex_search(this->c_str(), matches, expression))
        return false;

    for (uint i=1; i < matches.size(); i++)
    {
        matching_strings.push_back(std::string(matches[i].first, matches[i].second));
    }
    return true;
}

int FString::LineSplit(std::vector<FString> &lines, bool trim) const
{
    size_t delim=0, len = length();
    lines.clear();
    if ((delim = find_first_of("\n\r", 0))==std::string::npos)
    {
        lines.push_back(*this); // no line endings found
        return 1;
    }
    else
    {
        const FString &str(*this);
        if (str[delim] == '\r' &&
            delim < len - 1 && // might be another character
            str[delim+1] == '\n')
            return Explode("\r\n", lines, trim);
        else if (str[delim] == '\r')
            return Explode("\r", lines, trim);
        else
            return Explode("\n", lines, trim);
    }
}
int FString::Explode(const char *delim, std::vector<std::string> &components, bool trim, const char* strip_chars) const
{
    size_t cur=0,next=0, dlen = strlen(delim);
    components.clear();
    while ((next = find(delim, cur))!=std::string::npos)
    {
        if (trim)
            components.push_back(Mid(cur, next-cur).Trim(strip_chars));
        else
            components.push_back(Mid(cur, next-cur));
        cur += next-cur+dlen;
    }
    if (trim)
        components.push_back(Mid(cur).Trim(strip_chars));
    else
        components.push_back(Mid(cur));
    return components.size();
}

int FString::Explode(const char *delim, std::vector<FString> &components, bool trim, const char* strip_chars) const
{
    size_t cur=0,next=0, dlen = strlen(delim);
    components.clear();
    while ((next = find(delim, cur))!=std::string::npos)
    {
        if (trim)
            components.push_back(Mid(cur, next-cur).Trim(strip_chars));
        else
            components.push_back(Mid(cur, next-cur));
        cur += next-cur+dlen;
    }
    if (trim)
        components.push_back(Mid(cur).Trim(strip_chars));
    else
        components.push_back(Mid(cur));
    return components.size();
}

int FString::DelimitedListToSet(const char *delim,
                                std::set<FString> &components,
                                bool trim,
                                const char* strip_chars) const
{
    size_t cur=0,next=0, dlen = strlen(delim);
    while ((next = find(delim, cur))!=std::string::npos)
    {
        if (trim)
            components.insert(Mid(cur, next-cur).Trim(strip_chars));
        else
            components.insert(Mid(cur, next-cur));

        cur += next-cur+dlen;
    }
    if (trim)
        components.insert(Mid(cur).Trim(strip_chars));
    else
        components.insert(Mid(cur));
    return components.size();
}

int FString::DelimitedListToSet(const char *delim,
                                std::set<std::string> &components,
                                bool trim,
                                const char* strip_chars) const
{
    size_t cur=0,next=0, dlen = strlen(delim);
    components.clear();
    while ((next = find(delim, cur))!=std::string::npos)
    {
        if (trim)
            components.insert(Mid(cur, next-cur).Trim(strip_chars));
        else
            components.insert(Mid(cur, next-cur));

        cur += next-cur+dlen;
    }
    if (trim)
        components.insert(Mid(cur).Trim(strip_chars));
    else
        components.insert(Mid(cur));
    return components.size();
}

int FString::Tokenize(const char *delim, std::vector<FString> &components, size_t max_parts) const
{
    size_t start = 0, end = 0;
    size_t i = 0;
    components.clear();

    while (1)
    {
        start = find_first_not_of(delim, start);
        if (std::string::npos == start)
            break;
        end = find_first_of(delim, start);
        if (std::string::npos == end)
        {
            components.push_back(Mid(start));
            break;
        }

        if (++i == max_parts) break;

        if (end - start > 0)
            components.push_back(Mid(start, end - start));
        start = end;
    }

    if (max_parts != 0 && i == max_parts)
    {
        // we reached the max parts and breaked
        components.push_back(Mid(start));
    }

    return components.size();
}

int FString::Tokenize(const char *delim, std::vector<std::string> &components, size_t max_parts) const
{
    size_t start = 0, end = 0;
    size_t i = 0;
    components.clear();

    while (1)
    {
        start = find_first_not_of(delim, start);
        if (std::string::npos == start)
            break;
        end = find_first_of(delim, start);
        if (std::string::npos == end)
        {
            components.push_back(Mid(start));
            break;
        }

        if (++i == max_parts) break;

        if (end - start > 0)
            components.push_back(Mid(start, end - start));
        start = end;
    }

    if (max_parts != 0 &&
        i == max_parts)
    {
        // we reached the max parts and breaked
        components.push_back(Mid(start));
    }

    return components.size();
}

#define CLEAN_CHAR_VECTOR(vec)		\
    foreach (const char *str, vec)	\
        if (str)			\
            free(reinterpret_cast<void*>(const_cast<char*>(str)));      \
    vec.clear();

int FString::Tokenize(const char *delim, std::vector<char*> &components, size_t max_parts) const
{
    size_t start = 0, end = 0;
    size_t i = 0;
    char *tmp;
    CLEAN_CHAR_VECTOR(components);

    while (1)
    {
        start = find_first_not_of(delim, start);
        if (std::string::npos == start)
            break;
        end = find_first_of(delim, start);
        if (std::string::npos == end)
        {
            if ((tmp = strdup(Mid(start).c_str())) == NULL)
            {
                CLEAN_CHAR_VECTOR(components);
                break;
            }
            components.push_back(tmp);
            break;
        }

        if (++i == max_parts) break;

        if (end - start > 0)
        {
            if ((tmp = strdup(Mid(start, end - start).c_str())) == NULL)
            {
                CLEAN_CHAR_VECTOR(components);
                break;
            }
            components.push_back(tmp);
        }
        start = end;
    }

    if (max_parts != 0 &&
        i == max_parts)
    {
        // we reached the max parts and breaked
        if ((tmp = strdup(Mid(start).c_str())) == NULL)
        {
            CLEAN_CHAR_VECTOR(components);
        }
        else
            components.push_back(tmp);
    }

    return components.size();
}

int FString::Tokenize(const char *delim, std::vector<const char*> &components, size_t max_parts) const
{
    std::vector<char*> result;
    CLEAN_CHAR_VECTOR(components);
    Tokenize(delim, result, max_parts);
    foreach (char *str, result)
        components.push_back(str);
    return components.size();
}

FString& FString::Implode(const char *glue, const std::vector<FString> &components, bool quotes)
{
    clear();
    std::vector<FString>::const_iterator i;
    bool first = true;
    for (i = components.begin(); i != components.end(); ++i)
    {
        if (!first) append(glue);
        if (quotes) append("'");
        append(*i);
        if (quotes) append("'");
        first = false;
    }
    return *this;
}

FString& FString::Implode(const char *glue, const std::vector<std::string> &components, bool quotes)
{
    clear();
    std::vector<std::string>::const_iterator i;
    bool first = true;
    for (i = components.begin(); i != components.end(); ++i)
    {
        if (!first) append(glue);
        if (quotes) append("'");
        append(*i);
        if (quotes) append("'");
        first = false;
    }
    return *this;
}


FString& FString::Implode(const char *glue, const std::set<FString> &components, bool quotes)
{
    clear();
    std::set<FString>::const_iterator i;
    bool first = true;
    for (i = components.begin(); i != components.end(); ++i)
    {
        if (!first) append(glue);
        if (quotes) append("'");
        append(*i);
        if (quotes) append("'");
        first = false;
    }
    return *this;
}

FString& FString::Implode(const char *glue, const std::list<FString> &components, bool quotes)
{
    clear();
    std::list<FString>::const_iterator i;
    bool first = true;
    for (i = components.begin(); i != components.end(); ++i)
    {
        if (!first) append(glue);
        if (quotes) append("'");
        append(*i);
        if (quotes) append("'");
        first = false;
    }
    return *this;
}

std::vector<FString> FString::split(const char *separator, size_t max_parts) const
{
    std::string::size_type pos, start = 0;
    std::string::size_type sep_len = strlen(separator);
    std::vector<FString> ret;
    size_t i = 0;

    while ((pos = find(separator, start)) != NOPOS)
    {
        if (++i == max_parts) break;
        ret.push_back(substr(start, pos - start));
        start = pos + sep_len;
    }

    ret.push_back(substr(start));
    return ret;
}


FString FString::Join(const std::vector<FString>& array, const char *separator)
{
    std::vector<FString>::const_iterator vi;
    FString str;

    for (vi = array.begin(); vi != array.end(); ++vi)
    {
        if (!str.empty()) str += separator;
        str += *vi;
    }

    return str;
}


FString FString::Join(const std::vector<std::string>& array, const char *separator)
{
    std::vector<std::string>::const_iterator vi;
    FString str;

    for (vi = array.begin(); vi != array.end(); ++vi)
    {
        if (!str.empty()) str += separator;
        str += *vi;
    }

    return str;
}


FString FString::Join(const std::vector<const char*>& array, const char *separator)
{
    std::vector<const char*>::const_iterator vi;
    FString str;

    for (vi = array.begin(); vi != array.end(); ++vi)
    {
        if (!str.empty()) str += separator;
        str += *vi;
    }

    return str;
}



FString & FString::LoadFile(const char *filename, unsigned int max_len, FString &out)
{
    unsigned int len;
    FILE *file;

    if (filename == NULL || strlen(filename) == 0)
    {
        hlog(HLOG_DEBUG, "FString failed load a file with an empty filename");
        throw EFStringLoadFile("invalid filename");
    }


    if ((file = fopen(filename, "r")) == NULL)
    {
        hlog(HLOG_DEBUG, "FString failed load file: %s", filename);
        throw EFStringLoadFile(FStringFC(),
                                             "unable to open file '%s'",
                                             filename);
    }

    // get file length
    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (len > max_len) len = max_len;

    // get file data
    char *str = new char [len];
    len = fread(str, 1, len, file);
    fclose(file);
    out.assign(str, len);
    delete [] str;
    return out;
}

void FString::SaveFile(const char *filename, const FString &in)
{
    FILE *file;

    if (filename == NULL || strlen(filename) == 0)
        throw EFString("invalid filename");

    if ((file = fopen(filename, "w")) == NULL)
        throw EFString(FStringFC(),
                                     "unable to open file '%s'", filename);
    size_t status = fwrite(in.c_str(), in.size(), 1, file);
    int err = errno;
    fclose(file);
    if (status < 1)
    {
        throw EFString(FStringFC(),
                       "failed to write to file '%s': %s",
                       filename,
                       SystemCallUtil::GetErrorDescription(err).c_str());
    }
}

FString FString::ShellEscape() const
{
    FString ret(*this);
    ret.Replace("'", "'\\''");
    return "'" + ret + "'";
}

#pragma GCC diagnostic ignored "-Wold-style-cast"

int FString::ExplodeBinary(std::vector<FString> &components) const
{
    components.clear();

    size_t pos = 0;
    size_t end = size();
    while (pos != end && (end - pos) >= sizeof(uint16_t))
    {
        uint16_t len;
        memcpy(&len, c_str() + pos, sizeof(len));
        len = ntohs(len);
        pos += sizeof(len);
        if (pos + len > end)
            len = end - pos;
        components.push_back(FString(c_str() + pos, len));
        pos += len;
    }
    return components.size();
}

/**
 * Takes a vector of FStrings and encodes them
 */
FString& FString::ImplodeBinary(const std::vector<FString> &components)
{
    clear();
    std::vector<FString>::const_iterator i;
    for (i = components.begin(); i != components.end(); ++i)
    {
        uint16_t len = i->size();
        len = htons(len);
        append(reinterpret_cast<char *>(&len), sizeof(len));
        append(*i);
    }
    return *this;
}

#pragma GCC diagnostic warning "-Wold-style-cast"

void FString::OFormat(const FString &format, const std::vector<std::string> &parameters)
{
    clear();

    size_t size = format.length();
    size = size + (2 * parameters.size());
    foreach(const std::string &parameter, parameters)
    {
        size += parameter.size();
    }
    reserve(size);

    size_t startPos = 0;
    size_t endPos = format.find_first_of("%@");
    std::vector<std::string>::const_iterator it = parameters.begin();

    while (endPos != npos && it != parameters.end())
    {
        append(format, startPos, endPos - startPos);
        append(*it);
        it++;
        startPos = endPos + 2;
        endPos = format.find_first_of("%@", startPos);
    }
    if (startPos < size)
        append(format, startPos, size - startPos);

}

void FString::OFormat(const FString &format, const std::vector<FString> &parameters)
{
    clear();

    size_t size = format.length();
    size = size + (2 * parameters.size());
    foreach(const FString &parameter, parameters)
    {
        size += parameter.size();
    }
    reserve(size);

    size_t startPos = 0;
    size_t endPos = format.find_first_of("%@");
    std::vector<FString>::const_iterator it = parameters.begin();

    while (endPos != npos && it != parameters.end())
    {
        append(format, startPos, endPos - startPos);
        append(*it);
        it++;
        startPos = endPos + 2;
        endPos = format.find_first_of("%@", startPos);
    }

    if (startPos < size)
        append(format, startPos, size - startPos);
}

FString FString::HexDump() const
{
    size_t n = length();
    FString result;
    result.reserve(3 * n + 1);

    size_t resultLen = 0;
    char digit[4];

    for (size_t i=0; i<n; i++, resultLen+=3)
    {
        snprintf(digit, 4, "%02X ", static_cast<unsigned char>(at(i)));
        result[resultLen] = digit[0];
        result[resultLen+1] = digit[1];
        result[resultLen+2] = digit[2];
    }
    result[resultLen] = '\0';
    return result;
}

