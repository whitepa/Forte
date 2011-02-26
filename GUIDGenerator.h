// GUID.h
#ifndef __forte_guidgenerator_h__
#define __forte_guidgenerator_h__

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "FString.h"
#include "RandomGenerator.h"

namespace Forte
{
    class GUIDGenerator : public Object
    {
    public:
        GUIDGenerator();

        FString & GenerateGUID(FString &out, bool pathSafe = false);

    private:
        RandomGenerator mRG;
        // we override the default basic_random_generator with a
        // mt19937 random generator to avoid valgrind errors.
        boost::mt19937 mRand;
        boost::uuids::basic_random_generator<boost::mt19937> mUUIDGen;
    };
};
#endif
