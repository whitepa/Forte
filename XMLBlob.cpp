// XMLBlob.cpp
#ifndef FORTE_NO_XML

#include "XMLBlob.h"
#include "Exception.h"
#include <wctype.h>

CXMLBlob::CXMLBlob(const char *rootName
    ) :
    readOnly(false)
{
    doc = xmlNewDoc(BAD_CAST "1.0");

    // create the root node
    root = xmlNewNode(NULL, BAD_CAST rootName);
    xmlDocSetRootElement(doc, root);

    current = root;
    lastData=NULL;
}

CXMLBlob::~CXMLBlob()
{
    xmlFreeDoc(doc);
}

CXMLBlob::CXMLBlob(const FString &in) :
    readOnly(true)
{
    // parse the incoming XML string
    throw CForteUnimplementedException("XML parsing not implemented");
}

void CXMLBlob::beginChild(const char *name)
{
    current = xmlNewChild(current, NULL, BAD_CAST name, NULL);
}

void CXMLBlob::endChild(void)
{
    if (current != NULL)
        current = current->parent;
}

void CXMLBlob::addAttribute(const char *name, const char *value)
{
    FString stripped;
    stripControls(stripped, value);
    xmlNewProp(current, BAD_CAST name, BAD_CAST stripped.c_str());
}

void CXMLBlob::addDataAttribute(const char *name, const char *value)
{
    if (lastData)
    {
        FString stripped;
        stripControls(stripped, value);
        xmlNewProp(lastData, BAD_CAST name, BAD_CAST stripped.c_str());
    }
}

void CXMLBlob::addData(const char *name, const char *value)
{
    FString stripped;
    addDataToNode(current, name, value);
}
void CXMLBlob::addDataRaw(const char *name, const char *value)
{
    FString stripped;
    addDataToNodeRaw(current, name, value);
}

void CXMLBlob::addDataToNode(xmlNodePtr node, const char *name, const char *value)
{
    FString stripped;
    stripControls(stripped, value);
    lastData = xmlNewTextChild(current, NULL, BAD_CAST name, BAD_CAST stripped.c_str());
}
void CXMLBlob::addDataToNodeRaw(xmlNodePtr node, const char *name, const char *value)
{
    FString stripped;
    lastData = xmlNewTextChild(current, NULL, BAD_CAST name, BAD_CAST value);
}

void CXMLBlob::toString(FString &out,
                        bool pretty
    )
{
    xmlChar *buf;
    int bufsize;
    xmlDocDumpFormatMemoryEnc(doc, &buf, &bufsize, "UTF-8", (pretty) ? 1 : 0);
    out.assign((const char *)buf, bufsize);
    xmlFree(buf);
}

void CXMLBlob::stripControls(FString &dest,  
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

#endif // FORTE_NO_XML
