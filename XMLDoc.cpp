// XMLDoc.cpp
#ifndef FORTE_NO_XML

#include "XMLDoc.h"
#include "LogManager.h"

using namespace Forte;

Mutex XMLDoc::sMutex;

XMLDoc::XMLDoc()
{
    hlog(HLOG_DEBUG4, "Locking XML mutex");
    sMutex.lock();
    mDoc = NULL;
}


XMLDoc::XMLDoc(const FString& xml)
{
    hlog(HLOG_DEBUG4, "Locking XML mutex");
    sMutex.lock();
    mDoc = xmlReadMemory(xml.c_str(), xml.length(), "noname.xml", NULL, 0);

    if (mDoc == NULL)
    {
        hlog(HLOG_DEBUG4, "Unlocking XML mutex");
        sMutex.unlock();
        throw ForteXMLDocException("FORTE_XML_PARSE_ERROR");
    }
}


XMLDoc::~XMLDoc()
{
    if (mDoc != NULL) xmlFreeDoc(mDoc);
    xmlCleanupParser();
    hlog(HLOG_DEBUG4, "Unlocking XML mutex");
    sMutex.unlock();
}


XMLNode XMLDoc::createDocument(const FString& root_name)
{
    xmlNodePtr root;
    if (mDoc != NULL) xmlFreeDoc(mDoc);
    mDoc = xmlNewDoc(BAD_CAST "1.0");
    root = xmlNewNode(NULL, BAD_CAST root_name.c_str());
    xmlDocSetRootElement(mDoc, root);
    return root;
}


FString XMLDoc::toString() const
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
