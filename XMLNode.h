#ifndef __forte_XMLNode_h
#define __forte_XMLNode_h

#ifndef FORTE_NO_XML

#include "FString.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
namespace Forte
{
    class XMLNode;

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
        FString getProp(const char *name) const;
        void setProp(const char *name, const char *value = NULL);
        void delProp(const char *name);
        FString getText() const;
        void nuke();   // detach and delete this node and all of its subchildren

        inline FString getName() const { return mNode == NULL ? "" : FString((const char*)mNode->name); }
        inline xmlNodePtr getChildren() const { return mNode == NULL ? NULL : mNode->children; }
        inline xmlNodePtr getNext() const { return mNode == NULL ? NULL : mNode->next; }
        inline xmlNodePtr getParent() const { return mNode == NULL ? NULL : mNode->parent; }
        inline operator xmlNodePtr() { return mNode; }
        inline operator const xmlNodePtr() const { return mNode; }

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
