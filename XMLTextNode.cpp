// XMLTextNode.cpp
#ifndef FORTE_NO_XML

#include "XMLTextNode.h"

using namespace Forte;

CXMLTextNode::CXMLTextNode(const FString& name, const FString& text, xmlNodePtr parent) :
    CXMLNode()
{
    FString stripped;
    stripControls(stripped, text);
    m_node = xmlNewTextChild(parent, NULL, BAD_CAST name.c_str(), BAD_CAST stripped.c_str());
}

#endif
