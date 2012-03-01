// XMLDoc.cpp
#ifndef FORTE_NO_XML

#include "XMLDoc.h"
#include "LogManager.h"
#include <libxml/xmlsave.h>

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
    xmlBufferPtr buf = xmlBufferCreate();
    if (!buf)
        throw ForteXMLDocException("Could Not Allocate buffer");

    xmlSaveCtxtPtr savePtr = xmlSaveToBuffer(buf, "UTF-8", XML_SAVE_FORMAT | XML_SAVE_NO_DECL);
    if (!savePtr)
    {
        xmlBufferFree(buf);
        throw ForteXMLDocException("Could Not Save");
    }

    xmlSaveDoc(savePtr, mDoc);
    xmlSaveClose(savePtr);
    ret.assign((const char *) buf->content, buf->use);

    xmlBufferFree(buf);
    return ret;
}

#endif
