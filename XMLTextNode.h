#ifndef __forte_XMLTextNode
#define __forte_XMLTextNode

#ifndef FORTE_NO_XML

#include "XMLNode.h"

class CXMLTextNode : public CXMLNode
{
public:
    CXMLTextNode(const FString& name, const FString& text, xmlNodePtr parent = NULL);
    virtual ~CXMLTextNode() { }
};

#endif
#endif
