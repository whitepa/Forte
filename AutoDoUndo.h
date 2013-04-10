#ifndef __forte_auto_do_undo_h
#define __forte_auto_do_undo_h

#include "Object.h"
#include <boost/function.hpp>

namespace Forte
{
    class AutoDoUndo : public Object
    {
    public:
        typedef boost::function<void()> DoFunction;
        typedef boost::function<void()> UndoFunction;

        AutoDoUndo(DoFunction dofunc,
                   UndoFunction undofunc)
            :mUndo(undofunc)
        {
            dofunc();
        }

        AutoDoUndo(UndoFunction undofunc)
            :mUndo(undofunc)
        {
        }

        ~AutoDoUndo()
        {
            try
            {
                mUndo();
            }
            catch (std::exception& e)
            {
                hlog(HLOG_ERR, "Got exception %s", e.what());
            }
            catch(...)
            {
                hlog(HLOG_ERR, "Got unknown exception");
            }
        }

        const boost::function<void()> mUndo;
    };
};

#endif
