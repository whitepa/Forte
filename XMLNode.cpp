// XMLNode.cpp
#ifndef FORTE_NO_XML

#include "XMLNode.h"
#include <wctype.h>

using namespace Forte;

CXMLNode::CXMLNode(const FString& name, xmlNodePtr parent)
{
    FString stripped;
    stripControls(stripped, name);
    m_node = xmlNewChild(parent, NULL, BAD_CAST stripped.c_str(), NULL);
}


CXMLNode::~CXMLNode()
{
}


FString CXMLNode::getProp(const char *name) const
{
    FString ret;
    xmlChar *str;

    str = xmlGetProp(m_node, BAD_CAST name);

    if (str != NULL)
    {
        ret = (const char*)str;
        xmlFree(str);
    }

    return ret;
}


void CXMLNode::setProp(const char *name, const char *value)
{
    FString stripped;
    stripControls(stripped, value);
    xmlSetProp(m_node, BAD_CAST name, BAD_CAST stripped.c_str());
}


void CXMLNode::delProp(const char *name)
{
    xmlUnsetProp(m_node, BAD_CAST name);
}


FString CXMLNode::getText() const
{
    FString ret;
    xmlChar *str;

    str = xmlNodeGetContent(m_node);

    if (str != NULL)
    {
        ret = (const char*)str;
        xmlFree(str);
    }

    return ret;
}


void CXMLNode::nuke()
{
    xmlUnlinkNode(m_node);
    xmlFreeNode(m_node);
    m_node = NULL;
}


void CXMLNode::stripControls(FString &dest,  
                             const char *src 
    )
{
    if (src == NULL)
    {
        dest.assign("");
        return;
    }
    int wc_len = mbstowcs(NULL,src,0);
    wchar_t *wc_src = new wchar_t[wc_len+1];
    wchar_t *wc_dest = new wchar_t[wc_len+1];
    mbstowcs(wc_src, src, wc_len);
    int i = 0;
    int j = 0;
    for (; i < wc_len; i++)
    {
        if (!iswctype(wc_src[i], wctype("cntrl")))
            wc_dest[j++] = wc_src[i];
    }
    wc_dest[j] = 0;

    // convert back to mbs
    int len = wcstombs(NULL,wc_dest,0);
    char *pDest = new char[len+1];

    wcstombs(pDest, wc_dest, len);
    pDest[len] = 0;
    dest = pDest;
    delete [] pDest;
    delete [] wc_dest;
    delete [] wc_src;
}


#endif
