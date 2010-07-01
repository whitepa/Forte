#ifndef __ProcessManager_h
#define __ProcessManager_h

#include "Types.h"
#include "Object.h"
#include "ProcessHandler.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>


namespace Forte
{

    class ProcessManager : public Object
    {
    public:
        ProcessManager();
        virtual ~ProcessManager();
        
    protected:
        std::vector < boost::shared_ptr<ProcessHandler> > processHandlers;
    };

};
#endif
