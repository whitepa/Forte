// XMLNode.cpp
#ifndef FORTE_NO_XML

#include "XMLDoc.h"
#include "XMLNode.h"
#include "LogManager.h"
#include <boost/lexical_cast.hpp>
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
    mNode = xmlNewChild(parent, NULL, reinterpret_cast<xmlChar*>(const_cast<char*>(stripped.c_str())), NULL);
}


XMLNode::~XMLNode()
{
}

FString XMLNode::GetPath() const
{
    FString ret;
    xmlChar *str;

    str = xmlGetNodePath(mNode);
    if (str != NULL)
    {
        ret.assign(const_cast<const char*>(reinterpret_cast<char*>(str)));
        xmlFree(str);
    }
    return ret;
}

FString XMLNode::GetProp(const char *name) const
{
    FString ret;
    xmlChar *str;

    str = xmlGetProp(mNode, reinterpret_cast<xmlChar*>(const_cast<char*>(name)));

    if (str != NULL)
    {
        ret = const_cast<const char*>(reinterpret_cast<char*>(str));
        xmlFree(str);
    }

    return ret;
}


void XMLNode::SetProp(const char *name, const char *value)
{
    FString stripped;
    stripControls(stripped, value);
    xmlSetProp(mNode, reinterpret_cast<xmlChar*>(const_cast<char*>(name)), reinterpret_cast<xmlChar*>(const_cast<char*>(stripped.c_str())));
}


void XMLNode::DelProp(const char *name)
{
    xmlUnsetProp(mNode, reinterpret_cast<xmlChar*>(const_cast<char*>(name)));
}


FString XMLNode::GetText() const
{
    FString ret;
    xmlChar *str;

    str = xmlNodeGetContent(mNode);

    if (str != NULL)
    {
        ret = const_cast<const char*>(reinterpret_cast<char*>(str));
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

void XMLNode::XPathStr(FString &resultString, const Forte::FString &xpath)
{
    boost::shared_ptr<xmlXPathContext> ctxt(xmlXPathNewContext(mNode->doc),
            bind(xmlXPathFreeContext, _1));
    if (!ctxt)
        throw ForteXMLDocException("XPath query invalid");
    ctxt->node = mNode;

    boost::shared_ptr<xmlXPathObject> result(
        xmlXPathEval(reinterpret_cast<const xmlChar*>(xpath.c_str()),
                     ctxt.get()), bind(xmlXPathFreeObject, _1));

    if (!result)
        throw ForteXMLDocException("XPath query invalid");

    if (result->type == XPATH_NUMBER)
    {
        resultString = lexical_cast<FString>(result->floatval);
    }
    else if(result->type == XPATH_STRING)
    {
        resultString = FString(reinterpret_cast<char *>(result->stringval));
    }
    else if(result->type == XPATH_BOOLEAN)
    {
        //resultString = lexical_cast<FString>(result->boolval);
        resultString = "false";
        if (result->boolval)
            resultString = FString("true");
    }
    else if(result->type == XPATH_NODESET)
    {
        const int count = xmlXPathNodeSetGetLength(result->nodesetval);
        for (int i = 0; i != count; i++)
        {
            if (result->nodesetval->nodeTab[i]->type == XML_ATTRIBUTE_NODE)
            {
                if (!resultString.empty())
                    resultString.append("\n");
                // There should be only one child for each attribute node, a text
                resultString = resultString + FString(reinterpret_cast<char *>(result->nodesetval->nodeTab[i]->children->content));
                //resultString = resultString + FString((char *) xmlNodeGetContent(result->nodesetval->nodeTab[i]);
            }
            else if (result->nodesetval->nodeTab[i]->type == XML_TEXT_NODE)
            {
                if (!resultString.empty())
                    resultString.append("\n");
                resultString = resultString + FString(reinterpret_cast<char *>(result->nodesetval->nodeTab[i]->content));
            }
            else
            {
                hlog(HLOG_DEBUG, "found type: %i", result->nodesetval->nodeTab[i]->type);
                throw ForteXMLDocException("XPathStr nodeset result unsupported for non-attr, non-text nodes.");
            }
        }
    }
    else
    {
        throw ForteXMLDocException("XPath result type unknown");
    }
}

void XMLNode::Find(std::vector<XMLNode> &nodes, const Forte::FString &xpath)
{
    boost::shared_ptr<xmlXPathContext> ctxt(xmlXPathNewContext(mNode->doc),
            bind(xmlXPathFreeContext, _1));
    if (!ctxt)
        throw ForteXMLDocException("XPath query invalid");
    ctxt->node = mNode;

    boost::shared_ptr<xmlXPathObject> result(
        xmlXPathEval(reinterpret_cast<const xmlChar*>(xpath.c_str()),
                     ctxt.get()), bind(xmlXPathFreeObject, _1));

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
