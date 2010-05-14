#ifndef _UtilTest_h
#define _UtilTest_h

namespace Forte
{

    EXCEPTION_SUBCLASS(Exception, ForteUnilTestException);

    class TestRequestHandler : public RequestHandler
    {
    public:
        virtual ~TestRequestHandler() { }
        virtual void Handler(Event *e);
        virtual void Busy(void);
        virtual void Periodic(void);
        virtual void Init(void);
        virtual void Cleanup(void);
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
