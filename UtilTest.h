#ifndef _UtilTest_h
#define _UtilTest_h

namespace Forte
{

    EXCEPTION_SUBCLASS(ForteException, ForteUnilTestException);

    class TestRequestHandler : public RequestHandler
    {
    public:
        virtual ~TestRequestHandler() { }
        virtual void handler(Event *e);
        virtual void busy(void);
        virtual void periodic(void);
        virtual void init(void);
        virtual void cleanup(void);
    };

    class TestEvent : public Event
    {
    public:
        TestEvent();
        virtual ~TestEvent();
        virtual Event * copy() { TestEvent *e = new TestEvent; e->mStr = mStr; return e; }
        FString mStr;
    };
};
#endif
