#ifndef __forte_XMLTextNode
#define __forte_XMLTextNode

#ifndef FORTE_NO_XML

#include "XMLNode.h"
namespace Forte
{
    class XMLTextNode : public XMLNode
    {
    public:
        XMLTextNode(const FString& name, const FString& text,
                    xmlNodePtr parent = NULL, bool stripControlChars = true);
        virtual ~XMLTextNode() { }
    };
};
#endif
#endif
