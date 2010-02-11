#ifndef _UtilTest_h
#define _UtilTest_h

EXCEPTION_SUBCLASS(CForteException, CForteUnilTestException);

class CTestRequestHandler : public CRequestHandler
{
public:
    virtual ~CTestRequestHandler() { }
    virtual void handler(CEvent *e);
    virtual void busy(void);
    virtual void periodic(void);
    virtual void init(void);
    virtual void cleanup(void);
};

class CTestEvent : public CEvent
{
public:
    CTestEvent();
    virtual ~CTestEvent();
    virtual CEvent * copy() { CTestEvent *e = new CTestEvent; e->mStr = mStr; return e; }
    FString mStr;
};
#endif
