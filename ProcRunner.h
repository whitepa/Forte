#ifndef _ProcRunner_h
#define _ProcRunner_h

#include "Types.h"
#include "AutoMutex.h"

class ProcRunner
{
public:
    // ctor/dtor
    virtual ~ProcRunner() { }
    
    // singleton
    static ProcRunner* get();
    static ProcRunner& getRef();
    static ProcRunner* s_singleton;
    static CMutex s_mutex;

public:
    // API
    // timeout: -1 = default, 0 = none
    virtual int run(const FString& command, 
                    const FString& cwd = "", 
                    FString *output = NULL,
                    int timeout = -1, 
                    const StrStrMap *env = NULL);

    virtual int run_background(const FString& command, 
                               const FString& cwd = "",
                               const StrStrMap *env = NULL);

    virtual FString read_pipe(const FString& command, int *exitval = NULL);

    virtual int rsh(const FString& ip, 
                    const FString& command, 
                    FString *output = NULL,
                    int timeout = -1);

    virtual int ssh(const FString& ip, 
                    const FString& command, 
                    FString *output = NULL,
                    int timeout = -1);

    virtual FString shell_escape(const FString& arg);
};

// inline funcs for legacy code
inline FString shell_escape(const FString& arg)
{
    return ProcRunner::get()->shell_escape(arg);
}

inline FString read_pipe(const FString& command, int *exitval = NULL)
{
    return ProcRunner::get()->read_pipe(command, exitval);
}

#endif
