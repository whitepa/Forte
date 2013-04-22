#ifndef __Forte__GMockClock_h
#define __Forte__GMockClock_h

#include "Clock.h"

namespace Forte
{
    class GMockClock : public Forte::Clock
    {
    public:
        MOCK_CONST_METHOD1(GetTime, void (struct timespec&));
        MOCK_CONST_METHOD1(ConvertToRealtime,
                           Timespec(const struct timespec &ts));
        MOCK_CONST_METHOD1(ConvertToRealtimePreserveZero,
                           Timespec(const struct timespec &ts));

    };
};
#endif
