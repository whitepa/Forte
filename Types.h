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
#include "FString.h"
#include "Exception.h"
#include "Foreach.h"

// types
namespace Forte
{
    typedef std::vector<unsigned> UIntVector;
    typedef std::set<unsigned> UIntSet;
    typedef std::vector<FString> StrList;
    typedef std::map<FString, FString> StrStrMap;
    typedef std::map<FString, FString> FStringFStringMap;
    typedef std::map<FString, int> StrIntMap;
    typedef std::map<int, FString> IntStrMap;
    typedef std::pair<int, FString> IntStrPair;
    typedef std::pair<FString, FString> StrStrPair;
    typedef std::map<FString, double> StrDoubleMap;
    typedef std::set<FString> FStringSet;
    typedef std::vector<FString> FStringVector;
    typedef std::list<FString> FStringList;
};

#endif
