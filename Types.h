#ifndef __Types_h
#define __Types_h

// for casting things in printf-like functions
#define u64 unsigned long long int

// includes
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <ext/hash_map>
#include "FString.h"
#include "Exception.h"
#include "Foreach.h"

// namespace
using namespace std;
using namespace __gnu_cxx;

struct eqstr
{
    bool operator()(const char* s1, const char* s2) const
        {
            return strcmp(s1, s2) == 0;
        }
};

// types
namespace Forte
{
    typedef vector<unsigned> UIntVector;
    typedef set<unsigned> UIntSet;
    typedef hash_map<FString, FString, hash<const char *>, eqstr> StringHashMap;
    typedef vector<FString> StrList;
    typedef map<FString, FString> StrStrMap;
    typedef map<FString, int> StrIntMap;
    typedef map<int, FString> IntStrMap;
    typedef pair<int, FString> IntStrPair;
    typedef pair<FString, FString> StrStrPair;
    typedef map<FString, double> StrDoubleMap;
    typedef set<FString> FStringSet;
    typedef vector<FString> FStringVector;
};

#endif
