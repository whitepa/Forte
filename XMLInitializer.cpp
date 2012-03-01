#include "XMLInitializer.h"
#include <libxml/parser.h>

namespace Forte {

XMLInitializer::XMLInitializer()
{
    xmlInitParser();
}
XMLInitializer::~XMLInitializer()
{
    xmlCleanupParser();
}

} /* namespace Forte */
