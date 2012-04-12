// XMLTextNode.cpp
#ifndef FORTE_NO_XML

#include "XMLTextNode.h"

using namespace Forte;

XMLTextNode::XMLTextNode(const FString& name, const FString& text,
                         xmlNodePtr parent, bool stripControlChars) :
    XMLNode()
{
    FString stripped;
    if (stripControlChars)
    {
        stripControls(stripped, text);
    }
    else
    {
        stripped = text;
    }
    mNode = xmlNewTextChild(parent, NULL, BAD_CAST name.c_str(), BAD_CAST stripped.c_str());
}

#endif
