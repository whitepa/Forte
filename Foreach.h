#ifndef __ForteForeach_h_
#define __ForteForeach_h_

#include <boost/foreach.hpp>

namespace boost
{
    // Suggested work-around for https://svn.boost.org/trac/boost/ticket/6131
    namespace BOOST_FOREACH = foreach;
}

#define foreach BOOST_FOREACH

#endif
