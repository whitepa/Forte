#ifndef __forte_XMLNode_h
#define __forte_XMLNode_h

#ifndef FORTE_NO_XML

#include "FString.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
namespace Forte
{

    class XMLNode
    {
    public:
        XMLNode() : mNode(NULL) { }
        XMLNode(xmlNodePtr node) : mNode(node) { *this = node; }
        XMLNode(const FString& name, xmlNodePtr parent = NULL);
        XMLNode(const XMLNode& other) : mNode(other.mNode) { *this = other; }
        XMLNode& operator =(const XMLNode& other) { mNode = other.mNode; return *this; }
        XMLNode& operator =(const xmlNodePtr node) { mNode = node; return *this; }
        virtual ~XMLNode();

    public:
        FString GetProp(const char *name) const;
        void SetProp(const char *name, const char *value = NULL);
        void DelProp(const char *name);
        FString GetText() const;
        FString GetPath() const;

        void Nuke();   // detach and delete this node and all of its subchildren

        inline bool IsValid() const { return mNode != NULL; }
        inline FString GetName() const { return mNode == NULL ? "" : FString(reinterpret_cast<const char*>(mNode->name)); }
        //inline xmlElementType GetType() const { return mNode != NULL ? mNode->type : throw Exception(); }
        inline xmlNodePtr GetChildren() const { return mNode == NULL ? NULL : mNode->children; }
        inline xmlNodePtr GetNext() const { return mNode == NULL ? NULL : mNode->next; }
        inline xmlNodePtr GetParent() const { return mNode == NULL ? NULL : mNode->parent; }
        void Find(std::vector<XMLNode> &nodes, const Forte::FString &xPath);
        void XPathStr(FString & resultString, const Forte::FString &xPath);

        inline operator xmlNodePtr() { return mNode; }
        inline operator const xmlNodePtr() const { return mNode; }
        inline operator const bool() const { return IsValid(); }

     protected:
        friend class XMLDoc;

        ///
        /// Strip all control characters from a string, even if the src is a MBS (UTF-8 string)
        static void stripControls(FString &dest,   ///< destination string
                                  const char *src  ///< Source string, ASCII or MBS
            );

        xmlNodePtr mNode;
    };
};
#endif
#endif
