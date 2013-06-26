#ifndef __Forte_FunctionThread_h__
#define __Forte_FunctionThread_h__

namespace Forte
{
    class FunctionThread : public Forte::Thread
    {
    public:
        FunctionThread(boost::function<void()> f,
                       const std::string& name)
            : mFunction(f) {
            setThreadName(name);
        }

        class AutoInit {};

        FunctionThread(const AutoInit& throwaway,
                       boost::function<void()> f,
                       const std::string& name)
            : mFunction(f) {
            setThreadName(name);
            initialized();
        }

        ~FunctionThread() {
            deleting();
        }

        void Begin() {
            initialized();
        }

    private:
        void* run() {
            mFunction();
            return NULL;
        }

    private:
        boost::function<void()> mFunction;
    };
}

#endif /* __Forte_FunctionThread_h__ */

