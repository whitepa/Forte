// RandomGenerator.h
#ifndef __forte_random_generator_h__
#define __forte_random_generator_h__

#include "FString.h"
#include "Exception.h"
#include "AutoFD.h"

namespace Forte
{
    EXCEPTION_CLASS(ERandomGenerator);
    EXCEPTION_SUBCLASS2(ERandomGenerator, ERandomGeneratorDevice,
                        "Unable to open random generator device");
    EXCEPTION_SUBCLASS2(ERandomGenerator, ERandomGeneratorFailed,
                        "Failed to read random data");

    class RandomGenerator : public Object
    {
    public:
        /** 
         * Create a random number generation object. /dev/urandom is
         * used as the random source.
         * 
         */
        RandomGenerator();
        virtual ~RandomGenerator() {};

        /** 
         * GetRandomData will return a string containing random binary
         * data of the specified length.
         * 
         * @param length bytes
         * @param out reference to output string object
         * 
         * @return FString containing length bytes of random data.
         */
        FString & GetRandomData(unsigned int length, FString &out);

        /** 
         * GetRandomUInt will return a random unsigned integer.
         * 
         * 
         * @return random unsigned int
         */
        unsigned int GetRandomUInt(void);

    private:
        AutoFD mRandomDevice;
    };
};
#endif
