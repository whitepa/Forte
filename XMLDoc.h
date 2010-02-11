#ifndef __forte_XMLDoc_h
#define __forte_XMLDoc_h

#ifndef FORTE_NO_XML

#include "AutoMutex.h"
#include "FString.h"
#include "XMLNode.h"
#include "XMLTextNode.h"
#include "Exception.h"

EXCEPTION_SUBCLASS(CForteException, CForteXMLDocException);

class CXMLDoc
{
public:
    CXMLDoc();
    CXMLDoc(xmlDocPtr doc) : m_doc(doc) { }
    CXMLDoc(const FString& xml);
    virtual ~CXMLDoc();

public:
    inline CXMLNode getRootNode() const { return CXMLNode(m_doc->children); }
    CXMLNode createDocument(const FString& root_name);
    FString toString() const;

    inline operator CXMLNode() const { return getRootNode(); }
    inline operator FString() const { return toString(); }
    inline operator xmlDocPtr() { return m_doc; }
    inline operator const xmlDocPtr() const { return m_doc; }

protected:
    static CMutex s_mutex;  // lock for all libxml2 access
    xmlDocPtr m_doc;
};

#endif
#endif
