#include "CheckedString.h"
#include "CheckedInt32.h"
#include "EventPredicate.h"
#include "Foreach.h"
#include "FTrace.h"

using namespace Forte;

const int Forte::EventPredicate::AND   = 0;
const int Forte::EventPredicate::OR    = 1;
const int Forte::EventPredicate::NOT   = 2;

const int Forte::EventPredicate::EVENT_NAME_IS       = 3;

bool Forte::EventPredicate::Evaluate(const Forte::Event &e)
{
    FTRACE;
    if (mType == AND)
    {
        if (mOperands.empty())
            throw EEventPredicateInvalid("AND operator requires at least one operand");
        foreach (Forte::EventPredicatePtr p, mOperands)
            if (!p->Evaluate(e)) return false;
        return true;
    }
    else if (mType == OR)
    {
        if (mOperands.empty())
            throw EEventPredicateInvalid("OR operator requires at least one operand");
        foreach (Forte::EventPredicatePtr p, mOperands)
            if (p->Evaluate(e)) return true;
        return false;
    }
    else if (mType == NOT)
    {
        if (mOperands.size() != 1)
            throw EEventPredicateInvalid(FStringFC(),
                                    "NOT operator requires one operand (has %u)",
                                         static_cast<unsigned>(mOperands.size()));
        return !(mOperands[0]->Evaluate(e));
    }
    else if (mType == EVENT_NAME_IS)
    {
        return (e.mName == mArg1);
    }
    throw EEventPredicateInvalid(FStringFC(), "EventPredicate has invalid type %d", mType);
}
