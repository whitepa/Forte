// XMLNode.cpp
#ifndef FORTE_NO_XML

#include "XMLDoc.h"
#include "XMLNode.h"
#include <wctype.h>
#include <boost/bind.hpp>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

using namespace boost;
using namespace Forte;

XMLNode::XMLNode(const FString& name, xmlNodePtr parent)
{
    FString stripped;
    stripControls(stripped, name);
    mNode = xmlNewChild(parent, NULL, BAD_CAST stripped.c_str(), NULL);
}


XMLNode::~XMLNode()
{
}


FString XMLNode::GetProp(const char *name) const
{
    FString ret;
    xmlChar *str;

    str = xmlGetProp(mNode, BAD_CAST name);

    if (str != NULL)
    {
        ret = (const char*)str;
        xmlFree(str);
    }

    return ret;
}


void XMLNode::SetProp(const char *name, const char *value)
{
    FString stripped;
    stripControls(stripped, value);
    xmlSetProp(mNode, BAD_CAST name, BAD_CAST stripped.c_str());
}


void XMLNode::DelProp(const char *name)
{
    xmlUnsetProp(mNode, BAD_CAST name);
}


FString XMLNode::GetText() const
{
    FString ret;
    xmlChar *str;

    str = xmlNodeGetContent(mNode);

    if (str != NULL)
    {
        ret = (const char*)str;
        xmlFree(str);
    }

    return ret;
}


void XMLNode::Nuke()
{
    xmlUnlinkNode(mNode);
    xmlFreeNode(mNode);
    mNode = NULL;
}

void XMLNode::Find(std::vector<XMLNode> &nodes, const Forte::FString &xpath)
{
    shared_ptr<xmlXPathContext> ctxt(xmlXPathNewContext(mNode->doc),
            bind(xmlXPathFreeContext, _1));
    if (!ctxt)
        throw ForteXMLDocException("XPath query invalid");
    ctxt->node = mNode;

    shared_ptr<xmlXPathObject> result(
            xmlXPathEval((const xmlChar*) xpath.c_str(), ctxt.get()),
            bind(xmlXPathFreeObject, _1));

    if (!result)
        throw ForteXMLDocException("XPath query invalid");

    if (result->type != XPATH_NODESET)
        throw ForteXMLDocException();

    xmlNodeSetPtr nodeset = result->nodesetval;
    if( nodeset && !xmlXPathNodeSetIsEmpty(nodeset))
    {
      const int count = xmlXPathNodeSetGetLength(nodeset);
      nodes.reserve(count);

      for (int i = 0; i != count; ++i)
      {
        xmlNodePtr node = xmlXPathNodeSetItem(nodeset, i);
        if(node->type == XML_NAMESPACE_DECL)
        {
            boost::throw_exception(ForteXMLDocException("Got namespace decl node"));
        }
        nodes.push_back(node);
      }
    }
}

void XMLNode::stripControls(FString &dest,  
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
