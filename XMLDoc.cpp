// XMLDoc.cpp
#ifndef FORTE_NO_XML

#include "XMLDoc.h"
#include "LogManager.h"

using namespace Forte;

Mutex XMLDoc::s_mutex;

XMLDoc::XMLDoc()
{
    hlog(HLOG_DEBUG4, "Locking XML mutex");
    s_mutex.lock();
    m_doc = NULL;
}


XMLDoc::XMLDoc(const FString& xml)
{
    hlog(HLOG_DEBUG4, "Locking XML mutex");
    s_mutex.lock();
    m_doc = xmlReadMemory(xml.c_str(), xml.length(), "noname.xml", NULL, 0);

    if (m_doc == NULL)
    {
        hlog(HLOG_DEBUG4, "Unlocking XML mutex");
        s_mutex.unlock();
        throw ForteXMLDocException("FORTE_XML_PARSE_ERROR");
    }
}


XMLDoc::~XMLDoc()
{
    if (m_doc != NULL) xmlFreeDoc(m_doc);
    xmlCleanupParser();
    hlog(HLOG_DEBUG4, "Unlocking XML mutex");
    s_mutex.unlock();
}


XMLNode XMLDoc::createDocument(const FString& root_name)
{
    xmlNodePtr root;
    if (m_doc != NULL) xmlFreeDoc(m_doc);
    m_doc = xmlNewDoc(BAD_CAST "1.0");
    root = xmlNewNode(NULL, BAD_CAST root_name.c_str());
    xmlDocSetRootElement(m_doc, root);
    return root;
}


FString XMLDoc::toString() const
{
    FString ret;
    xmlChar *buf;
    int bufsize;

    xmlDocDumpFormatMemoryEnc(m_doc, &buf, &bufsize, "UTF-8", 0);
    ret.assign((const char *)buf, bufsize);
    xmlFree(buf);
    return ret;
}

#endif
