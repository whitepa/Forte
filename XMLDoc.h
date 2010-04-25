#ifndef __forte_XMLDoc_h
#define __forte_XMLDoc_h

#ifndef FORTE_NO_XML

#include "AutoMutex.h"
#include "FString.h"
#include "XMLNode.h"
#include "XMLTextNode.h"
#include "Exception.h"

namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, ForteXMLDocException);

    class XMLDoc
    {
    public:
        XMLDoc();
        XMLDoc(xmlDocPtr doc) : m_doc(doc) { }
        XMLDoc(const FString& xml);
        virtual ~XMLDoc();

    public:
        inline XMLNode getRootNode() const { return XMLNode(m_doc->children); }
        XMLNode createDocument(const FString& root_name);
        FString toString() const;

        inline operator XMLNode() const { return getRootNode(); }
        inline operator FString() const { return toString(); }
        inline operator xmlDocPtr() { return m_doc; }
        inline operator const xmlDocPtr() const { return m_doc; }

    protected:
        static Mutex s_mutex;  // lock for all libxml2 access
        xmlDocPtr m_doc;
    };
};
#endif
#endif
