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

    /**
     * Make sure to construct XMLInitializer in main thread.
     */
    class XMLDoc
    {
    public:
        XMLDoc();
        XMLDoc(xmlDocPtr doc) : mDoc(doc) { }
        XMLDoc(const FString& xml);
        virtual ~XMLDoc();

    public:
        inline XMLNode GetRootNode() const { return XMLNode(mDoc->children); }
        XMLNode CreateDocument(const FString& root_name);
        FString ToString() const;

        inline operator XMLNode() const { return GetRootNode(); }
        inline operator FString() const { return ToString(); }
        inline operator xmlDocPtr() { return mDoc; }
        inline operator const xmlDocPtr() const { return mDoc; }

    protected:
        xmlDocPtr mDoc;
    };
};
#endif
#endif
