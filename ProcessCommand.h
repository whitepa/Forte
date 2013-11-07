#ifndef __ProcessCommand_H__
#define __ProcessCommand_H__

#include "FString.h"
#include "ProcessManager.h"
#include "SSHRunner.h"
#include "LogManager.h"

namespace Forte
{

template <class Executor>
int Execute(Executor* executor,
            const Forte::FString& command,
            Forte::FString& output,
            Forte::FString& errorOutput);

template <>
inline int Execute<Forte::ProcessManager>(Forte::ProcessManager* executor,
                                   const Forte::FString& command,
                                   Forte::FString& output,
                                   Forte::FString& errorOutput)
{
    return executor->CreateProcessAndGetResult(
                command, output, errorOutput, false);
}

template <>
inline int Execute<Forte::SSHRunner>(Forte::SSHRunner* executor,
                              const Forte::FString& command,
                              Forte::FString& output,
                              Forte::FString& errorOutput)
{
    return executor->Run(command, &output, &errorOutput);
}


/**
    * @class ProcessCommand
    *
    * @brief This class encapsulates processing a process command and retrieving response
    *
    * template 'Request' can be derived from ProcessCommandRequest
    * template 'Response' can be derived from ProcessCommandResponse
    *
    */
template<class Request, class Response, class Executor=Forte::ProcessManager>
class ProcessCommand
{
public:
    typedef Request request_type;
    typedef Response response_type;

    /**
      * execute the request and process the response
      * @param pm  process manager
      * @return  the response structure as defined by the 'Response' template
      * @throws EProcessManager
      */
    virtual response_type operator()()
    {
        Forte::FString output;
        Forte::FString errorOutput;
        hlogstream(HLOG_DEBUG, mRequest);

        // don't throw when command returns non-zero status code
        int result(Execute<Executor>(mExecutor.get(), mRequest.AsString(), output, errorOutput));
        response_type response(output, errorOutput, result);
        hlogstream(HLOG_DEBUG, response);
        return response;
    }

    explicit ProcessCommand(boost::shared_ptr<Executor> executor)
        :mExecutor(executor), mRequest()
    {
    }

    template<typename A0>
    explicit ProcessCommand(boost::shared_ptr<Executor> executor,
                            const A0& a0)
        :mExecutor(executor), mRequest(a0)
    {
    }

    template<typename A0, typename A1>
    explicit ProcessCommand(boost::shared_ptr<Executor> executor,
                            const A0& a0, const A1& a1)
        :mExecutor(executor), mRequest(a0, a1)
    {
    }

    template<typename A0, typename A1, typename A2>
    explicit ProcessCommand(boost::shared_ptr<Executor> executor,
                            const A0& a0, const A1& a1, const A2& a2)
        :mExecutor(executor), mRequest(a0, a1, a2)
    {
    }

    template<typename A0, typename A1, typename A2, typename A3>
    explicit ProcessCommand(boost::shared_ptr<Executor> executor,
                            const A0& a0, const A1& a1, const A2& a2, const A3& a3)
        :mExecutor(executor), mRequest(a0, a1, a2, a3)
    {
    }

    template<typename A0, typename A1, typename A2, typename A3, typename A4>
    explicit ProcessCommand(boost::shared_ptr<Executor> executor,
                            const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4)
        :mExecutor(executor), mRequest(a0, a1, a2, a3, a4)
    {
    }

    template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
    explicit ProcessCommand(boost::shared_ptr<Executor> executor,
                            const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5)
        :mExecutor(executor), mRequest(a0, a1, a2, a3, a4, a5)
    {
    }

private:
    boost::shared_ptr<Executor> mExecutor;
    const request_type mRequest;
}; // ProcessCommand

/**
    * @class ProcessCommandData
    *
    * @brief This class encapsulates conversion to/from a process command
    *
    */
class ProcessCommandData
{
public:
    /**
      * returns the string as would be seen executing a command in shell or it's return
      */
    virtual Forte::FString AsString() const=0;

    virtual void PrintPretty(std::ostream& o) const
    {
        int status(-4);
        char* res(abi::__cxa_demangle(typeid(*this).name(), NULL, NULL, &status));
        if(status == 0)
        {
            o << res;
        }
        o << ": '" << AsString() << "'";
        free(res);
    }
};

/**
    * @class ProcessCommandRequest
    *
    * @brief This class encapsulates/facilitates the conversion to shell from user format
    */
class ProcessCommandRequest : public ProcessCommandData
{
public:
    virtual ~ProcessCommandRequest(){}
};

/**
    * @class ProcessCommandRequestBasic
    *
    * @brief Basic implementation of ProcessCommandRequest which just passes
    *        the string argument
    */
class ProcessCommandRequestBasic : public ProcessCommandRequest
{
public:
    explicit ProcessCommandRequestBasic(const Forte::FString& request)
        :mRequest(request)
    {
    }

    virtual Forte::FString AsString() const
    {
        return mRequest;
    }

protected:
    const Forte::FString mRequest;
};

/**
    * @class ProcessCommandResponse
    *
    * @brief This class encapsulates/facilitates the conversion from shell response
    *        to user format
    */
class ProcessCommandResponse : public ProcessCommandData
{
public:
    virtual ~ProcessCommandResponse(){}

    virtual Forte::FString AsString() const
    {
        return mResponse;
    }

    int GetReturnCode() const
    {
        return mReturnCode;
    }

    bool operator!() const
    {
        return (mReturnCode != 0);
    }

protected:
    explicit ProcessCommandResponse(const Forte::FString& response,
                                    const Forte::FString& errorResponse,
                                    int returnCode) :
        mResponse(response), mErrorResponse(errorResponse),
        mReturnCode(returnCode)
    {
    }

protected:
    const Forte::FString mResponse;
    const Forte::FString mErrorResponse;
    int mReturnCode;
};

/**
    * @class ProcessCommandResponseBasic
    *
    * @brief Basic implementation of ProcessCommandResponse which just passes
    *        the string argument to ctor
    */
class ProcessCommandResponseBasic : public ProcessCommandResponse
{
public:
    explicit ProcessCommandResponseBasic(const Forte::FString& response,
                                         const Forte::FString& errorResponse,
                                         int returnCode)
        :ProcessCommandResponse(response, errorResponse, returnCode)
    {
    }
};

inline std::ostream& operator<<(std::ostream& out, const Forte::ProcessCommandData& obj)
{
    obj.PrintPretty(out);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const Forte::ProcessCommandRequest& obj)
{
    obj.PrintPretty(out);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const Forte::ProcessCommandResponse& obj)
{
    obj.PrintPretty(out);
    return out;
}

} // namespace Forte

#endif // __ProcessCommand_H__
