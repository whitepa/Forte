// XMLTextNode.cpp
#ifndef FORTE_NO_XML

#include "XMLTextNode.h"

using namespace Forte;

XMLTextNode::XMLTextNode(const FString& name, const FString& text, xmlNodePtr parent) :
    XMLNode()
{
    FString stripped;
    stripControls(stripped, text);
    mNode = xmlNewTextChild(parent, NULL, BAD_CAST name.c_str(), BAD_CAST stripped.c_str());
}

#endif
