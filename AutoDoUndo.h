#ifndef __forte_auto_do_undo_h
#define __forte_auto_do_undo_h

#include "Object.h"
#include <boost/function.hpp>

namespace Forte
{
    template <typename T1, typename T2>
    class AutoDoUndo : public Object
    {
    public:
        AutoDoUndo(boost::function<T1()> dofunc,
                   boost::function<T2()> undofunc,
                   bool autoDo = true) 
        {
            if (autoDo)
            {
                dofunc();
            }
            Undo = undofunc;
            Do = dofunc;
        }
        virtual ~AutoDoUndo() 
        {
            Undo();
        }

        boost::function<T1()> Do;
        boost::function<T2()> Undo;
    };
};

#endif
