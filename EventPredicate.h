#ifndef __Forte_EventPredicate_h_
#define __Forte_EventPredicate_h_

#include "Event.h"
#include "FString.h"
#include "Exception.h"
#include <boost/shared_ptr.hpp>

namespace Forte
{
    EXCEPTION_CLASS(EEventPredicate);
    EXCEPTION_SUBCLASS(EEventPredicate, EEventPredicateInvalid);

    /**
     * EventPredicate describes a boolean function to be applied to an
     * event.  Each predicate can be a constraint or an operator with
     * multiple Predicate operands.
     *
     * Supported operators:
     * AND - requires 2 or more operands
     * OR  - requires 2 or more operands
     * NOT - requires 1 operand
     *
     * Supported Contraints:
     * EVENT_TYPE_EQUALS
     *
     */

    class EventPredicate;
    typedef boost::shared_ptr<EventPredicate> EventPredicatePtr;

    class EventPredicate : public Object
    {
    public:
        EventPredicate(const EventPredicate &other) {
            *this = other;
        };
        const EventPredicate & operator= (const EventPredicate &rhs) {
            mType = rhs.mType;
            mArg1 = rhs.mArg1;
            mArg2 = rhs.mArg2;
            mOperands = rhs.mOperands;
            return *this;
        };
        EventPredicate(int type,
                         const FString &arg1) :
            mType(type), mArg1(arg1) {};
        EventPredicate(int type,
                         const FString &arg1,
                         const FString &arg2) :
            mType(type), mArg1(arg1), mArg2(arg2) {};
        EventPredicate(int type,
                         EventPredicatePtr op1) :
            mType(type)
            { mOperands.push_back(op1); }
        EventPredicate(int type,
                         EventPredicatePtr op1,
                         EventPredicatePtr op2) :
            mType(type)
            { mOperands.push_back(op1);mOperands.push_back(op2); }
        EventPredicate(int type,
                         EventPredicatePtr op1,
                         EventPredicatePtr op2,
                         EventPredicatePtr op3) :
            mType(type)
            { mOperands.push_back(op1);mOperands.push_back(op2);mOperands.push_back(op3); }

        virtual ~EventPredicate() {};

        inline operator EventPredicatePtr() const
            { return EventPredicatePtr(new EventPredicate(*this)); }


        virtual bool Evaluate(const Event &event);

        static const int AND;
        static const int OR;
        static const int NOT;

        static const int EVENT_NAME_IS;

        int mType;
        FString mArg1;
        FString mArg2;
        std::vector<EventPredicatePtr> mOperands;
    };
};

#endif
