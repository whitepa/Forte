// XMLDoc.cpp
#ifndef FORTE_NO_XML

#include "XMLDoc.h"
#include "LogManager.h"

using namespace Forte;

XMLDoc::XMLDoc()
{
    mDoc = NULL;
}


XMLDoc::XMLDoc(const FString& xml)
{
    mDoc = xmlReadMemory(xml.c_str(), xml.length(), "noname.xml", NULL, 0);

    if (mDoc == NULL)
    {
        throw ForteXMLDocException("FORTE_XML_PARSE_ERROR");
    }
}


XMLDoc::~XMLDoc()
{
    if (mDoc != NULL) xmlFreeDoc(mDoc);
}


XMLNode XMLDoc::CreateDocument(const FString& root_name)
{
    xmlNodePtr root;
    if (mDoc != NULL) xmlFreeDoc(mDoc);
    mDoc = xmlNewDoc(BAD_CAST "1.0");
    root = xmlNewNode(NULL, BAD_CAST root_name.c_str());
    xmlDocSetRootElement(mDoc, root);
    return root;
}


FString XMLDoc::ToString() const
{
    FString ret;
    xmlChar *buf;
    int bufsize;

    xmlDocDumpFormatMemoryEnc(mDoc, &buf, &bufsize, "UTF-8", 0);
    ret.assign((const char *)buf, bufsize);
    xmlFree(buf);
    return ret;
}

#endif
