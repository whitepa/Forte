// GUID.h
#ifndef __forte_guid_generator_h__
#define __forte_guid_generator_h__

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
        virtual ~GUIDGenerator() {};

        FString & GenerateGUID(FString &out);

    private:
        RandomGenerator mRG;
        // we override the default basic_random_generator with a
        // mt19937 random generator to avoid valgrind errors.
        boost::mt19937 mRand;
        boost::uuids::basic_random_generator<boost::mt19937> mUUIDGen;
    };
};
#endif
