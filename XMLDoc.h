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
        XMLDoc(xmlDocPtr doc) : mDoc(doc) { }
        XMLDoc(const FString& xml);
        virtual ~XMLDoc();

    public:
        inline XMLNode getRootNode() const { return XMLNode(mDoc->children); }
        XMLNode createDocument(const FString& root_name);
        FString to_string() const;

        inline operator XMLNode() const { return getRootNode(); }
        inline operator FString() const { return to_string(); }
        inline operator xmlDocPtr() { return mDoc; }
        inline operator const xmlDocPtr() const { return mDoc; }

    protected:
        static Mutex sMutex;  // lock for all libxml2 access
        xmlDocPtr mDoc;
    };
};
#endif
#endif
