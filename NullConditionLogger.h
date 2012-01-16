#ifndef __FORTE_NULL_CONDITION_LOGGER_h_
#define __FORTE_NULL_CONDITION_LOGGER_h_

#include "ConditionLogger.h"

namespace Forte
{
    class Condition;

    /**
     * \class NullConditionLogger
     **/
    class NullConditionLogger : public ConditionLogger
    {
        public:
            void Log (const ConditionLogEntry& condition)
            {
            }
    };
};

#endif
