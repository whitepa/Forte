// GUID.h
#ifndef __forte_guidgenerator_h__
#define __forte_guidgenerator_h__

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "AutoMutex.h"
#include "FString.h"
#include "RandomGenerator.h"

namespace Forte
{
    class GUIDGenerator : public Object
    {
    public:
        GUIDGenerator();
        virtual ~GUIDGenerator() {};

        virtual FString & GenerateGUID(FString &out);
        virtual void GenerateGUID(uint8_t out[]);
        virtual void GenerateGUID(std::vector<uint8_t> &out);

    private:
        Mutex mLock;
        RandomGenerator mRG;
        // we override the default basic_random_generator with a
        // mt19937 random generator to avoid valgrind errors.
        boost::mt19937 mRand;
        boost::uuids::basic_random_generator<boost::mt19937> mUUIDGen;
    };
};
#endif
