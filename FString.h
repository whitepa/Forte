#ifndef __FString_h
#define __FString_h

#include <string.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdarg>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <math.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define NOPOS std::string::npos
namespace Forte
{
    class FStringFC { // type for the FString Format Constructor 'flag' 
    public:
        FStringFC() {};
    }; 

    /**
     *The FString class provides a rich set of methods for handling strings. Use this class to
     *construct FStrings from other variable types as well as convert FStrings to other variable types.
     *Methods are also provided for trimming strings, connecting them, separating them out into 
     *appropriate parts, or selecting a portion of a string for other use. Be cautious when selecting
     *a method - some methods overwrite the existing string while others create brand new strings. This
     *information is documented accordingly. 
     **/
    class FString : public std::string
    {
    public:

        /**
         *Creates an empty FString.
         **/

        FString();

        /**
         *Overwrites this FString with a new FString containing the specified format. 
         **/

        void Format(const char *format, ...) __attribute__((format(printf,2,3)));

        /**
         *Constructs an FString from the sequence of data formatted as the format argument 
         *specifies using a variable argument list.
         **/

        void VFormat(const char *format, int size, va_list ap);

        /**
         *Constructs an FString from another FString. 
         **/

        FString(const FString& other) { static_cast<std::string&>(*this) = other; }

        /**
         *Constructs an FString from a standard string.
         **/

        FString(const std::string& other) { static_cast<std::string&>(*this) = other; }

        /**
         *Constructs an FString from a string.
         **/

        FString(const char *str) { if (str) static_cast<std::string&>(*this) = str; }

        /**
         *Constructs an FString containing the first len characters from the string str.  
         **/

        FString(const char *str, int len) : std::string(str, len) {};

        /**
         *Constructs an FString out of a character.
         **/

        FString(const char chr) { static_cast<std::string&>(*this) = chr; }

        /**
         *Constructs an FString from an unsigned integer.
         **/

        FString(unsigned int i) { Format("%u", i); }

        /**
         *Constructs an FString from an integer.
         **/
        FString(int i) { Format("%d", i); }

        /**
         *Constructs an FString from an unsigned long integer.
         **/
        FString(unsigned long i) { Format("%lu", i); }

        /**
         *Constructs an FString from a long long integer.
         **/
        FString(long long i) { Format("%lld", i); }

        /**
         *Constructs an FString from an unsigned long long integer. 
         **/
        FString(unsigned long long i) { Format("%llu", i); }

        /**
         *Constructs an FString of items in the vector delimited by a comma after each item.
         **/
        FString(const std::vector<unsigned int> &intvec);

        /**
         *Constructs an FString of items in the set delimited by a comma after each item.
         **/
        FString(const std::set<unsigned int> &intset);

        /**
         *Constructs an FString of items in the vector delimited by a comma after each item.
         **/
        FString(const std::vector<int> &intvec);

        /**
         *Constructs an FString of items in the set delimited by a comma after each item.
         **/
        FString(const std::set<int> &intset);

        /**
         *Constructs an FString of items in the set delimited by a comma after each item.
         **/
        FString(const std::set<std::string> &strset);

        /**
         *Constructs a string containing the IP address you are connected to in the sockaddr struct. 
         **/
        FString(const struct sockaddr *sa);

        /**
         *Constructs an FString from the sequence of data formatted as the format argument specifies.
         **/
        FString(const FStringFC &f, const char *format, ...)  __attribute__((format(printf,3,4)));
        virtual ~FString();

    public:
        // operators
        /**
         *Generates a null-terminated sequence of characters (c-string) with the same content as
         *the string object and returns it as a pointer to an array of characters.
         **/
        inline operator const char*() const { return c_str(); }

        inline char& operator [](int p) { return static_cast<std::string*>(this)->operator[](p); }

        // string helpers

        /**
         *Finds a string and replaces it with a selected string. Overwrites existing value.
         **/
        FString& Replace(const char* find, const char* replace);

        /**
         *Calls TrimLeft and TrimRight. @see TrimLeft. @see TrimRight.
         **/
        inline FString& Trim(const char *strip_char = " \t\r\n") { return TrimLeft(strip_char).TrimRight(strip_char); }
        /**
         *Removes strip_chars starting from the left until it hits a non strip_chars character.
         **/
        FString& TrimLeft(const char* strip_chars = " \t\r\n");

        /**
         *Removes strip_chars starting from the right until it hits a non strip_chars character.
         **/
        FString& TrimRight(const char* strip_chars = " \t\r\n");

        /**
         *Makes a string all uppercase. Overwrites existing value.
         **/
        FString& MakeUpper();

        /**
         *Makes a string all lowercase. Overwrites existing value.
         **/
        FString& MakeLower();
 
        /**
         *Returns the left n number of characters as a new FString.
         **/
        FString Left(int n) const { return substr(0, n); }

        /**
         *Creates a new string beginning at character p and continuing n places over.
         **/
        FString Mid(int p, int n = (int)NOPOS) const { if (p >= (int)length()) return ""; else return substr(p, n); }
  
        /**
         *Takes the right n characters, creates a brand new string and returns it to you.
         **/
        FString Right(int n) const { if (n > (int)length()) return *this; else return substr(length() - n, n); }

        /**
         *Compares two strings using strcmp. ( strcmp(c_str(), str); )
         **/ 
        inline int Compare(const char *str) const { return strcmp(c_str(), str); }
        inline int ComparePrefix(const char *str, int prefixLen) const { return strncmp(c_str(), str, prefixLen); }
        /**
         *Compares two strings using strcasecmp. ( strcasecmp(c_str(), str); )
         **/
        inline int CompareNoCase(const char *str) const {
            return strcasecmp(c_str(), str);
        }

        /**
         *Returns a shell escape'd FString for the object.
         **/
        FString ShellEscape() const;
        
        /**
         *Returns the FString as a double. 
         **/
        inline double AsDouble(void) const {
            //TODO: set errno to 0 before calling, check errno after the call
            return strtod(c_str(), NULL); 
        }

        /**
         *Returns the FString as an unsigned integer.
         **/
        inline unsigned int AsUnsignedInteger(void) const { return strtoul(c_str(), NULL, 0); }

        /**
         *Returns the FString as an integer.
         **/
        inline int AsInteger(void) const { return strtol(c_str(), NULL, 0); }

        /**
         *Returns the FString as an unsigned long long integer.
         **/
        inline unsigned long long AsUInt64(void) const { return strtoull(c_str(), NULL, 0); }

        /**
         *Returns the FString as an unsigned long integer.
         **/
        inline unsigned long AsUInt32(void) const { return strtoul(c_str(), NULL, 0); }

        /**
         *Returns the FString as an unsigned long long integer.
         **/
        inline long long AsInt64(void) const { return strtoll(c_str(), NULL, 0); }
       
        /**
         *Returns the FString as a long integer.
         **/
        inline long AsInt32(void) const { return strtol(c_str(), NULL, 0); }

        /**
         *Checks to see if the value in a string is a numeric value. @see strtod.
         **/
        inline bool IsNumeric(void) const
            {
                if (empty()) return false;
                char *c; 
                strtod(c_str(), &c); 
                return (*c == '\0'); 
            }

        /**
         *Checks to see if the value in a string is an unsigned numeric value. @see strtod.
         **/
        inline bool IsUnsignedNumeric(void) const
            {
                if (empty()) return false;
                char *c; 
                return strtod(c_str(), &c) >= 0.0 && (*c == '\0');
            }

        /// Split a multi-line string into single line components.  Line endings are
        /// automatically detected.  If trim is true, external whitespace will be trimmed
        /// from each line.  Line endings may be CR, LF, or CRLF, and must all be the same.

        /**
         *Splits the string on the new line character and returns a vector of FStrings 
         *representing each line. @param trim if true will trim the new line characters off of 
         *each line. @see Explode.
         **/
        int LineSplit(std::vector<FString> &lines, bool trim = false) const;

        /// Split a delimited string into its components.  Delimiters will not appear in
        /// the output.  If trim is true, external whitespace will be trimmed 
        /// from each component.

        /**
         *Creates a vector of FStrings by splitting the string on the delimiter you specify.
         *@param trim when true calls Trim(strip_chars) on each of the resulting strings. 
         **/
        int Explode(const char *delim, std::vector<FString> &components, 
                    bool trim=false, const char* strip_chars = " \t\r\n") const;

        /**
         *Creates a vector of strings by splitting the string on the delimiter you specify.
         *@param trim when true calls Trim(strip_chars) on each of the resulting strings.
         **/
        int Explode(const char *delim, std::vector<std::string> &components, 
                    bool trim=false, const char* strip_chars = " \t\r\n") const;

        //Similar to Explode, but does the actual tokenizing by skipping 
        //contiguous delimiter characters. Also, unlike Explode, each character
        //in delim can be a delimiter.
        int Tokenize(const char *delim, std::vector<FString> &components, size_t max_parts = 0) const;
        int Tokenize(const char *delim, std::vector<std::string> &components, size_t max_parts = 0) const;

        // TODO: make templated Implode functions
        /// Combine multiple components into a single string.  Glue will be used
        /// between each component.

        /**
         *Takes a vector of FStrings and generates a new FString using the provided delimiter. 
         *The new FString overwrites the pre-existing value of the FString specified. 
         **/
        FString& Implode(const char *glue, const std::vector<FString> &components, bool quotes = false);

        /**
         *Takes a vector of strings and generates a new FString using the provided delimiter. 
         *The new FString overwrites the pre-existing value of the FString specified. 
         **/
        FString& Implode(const char *glue, const std::vector<std::string> &components, bool quotes = false);
        
        /**
         *Takes a set of FStrings and generates a new FString using the provided delimiter. 
         *The new FString overwrites the pre-existing value of the FString specified. 
         **/
        FString& Implode(const char *glue, const std::set<FString> &components, bool quotes = false);

        /**
         *Takes a list of FStrings and generates a new FString using the provided delimiter. 
         *The new FString overwrites the pre-existing value of the FString specified. 
         **/
        FString& Implode(const char *glue, const std::list<FString> &components, bool quotes = false);

        /// Scale Computing's split() and Join(), similar to the above Implode() and Explode()
        std::vector<FString> split(const char *separator, size_t max_parts = 0) const;
       
        /**
         *Takes an array of FStrings and generates a brand new FString using the provided 
         *delimiter and returns it to you.  
         **/
        static FString Join(const std::vector<FString>& array, const char *separator);

        /**
         *Takes an array of strings and generates a brand new FString using the provided delimiter 
         *and returns it to you.  
         **/
        static FString Join(const std::vector<std::string>& array, const char *separator);

        /**
         *Takes a vector of strings and generates a brand new FString using the provided delimiter 
         *and returns it to you.  
         **/
        static FString Join(const std::vector<const char*>& array, const char *separator);

        // file I/O
        /**
         *Opens a file and loads a string from a file up to whatever you set for max_len. 
         **/
        static FString & LoadFile(const char *filename, unsigned int max_len, FString &out);

        /**
         *Saves an FString in the specified file. 
         **/
        static void SaveFile(const char *filename, const FString &in);
    };

    inline FString operator +(const FString& str, const FString& add) { FString ret = str; ret += add; return ret; }
    inline FString operator +(const FString& str, const char* add) { FString ret; ret = str; ret += add; return ret; }
    inline FString operator +(const char* add, const FString& str) { FString ret; ret = add; ret += str; return ret; }
    inline FString operator +(const FString& str, const char add) { FString ret; ret = str; ret += add; return ret; }
    inline FString operator +(const char add, const FString& str) { FString ret; ret = add; ret += str; return ret; }
};
#endif
