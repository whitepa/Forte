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
        XMLNode() : m_node(NULL) { }
        XMLNode(xmlNodePtr node) : m_node(node) { *this = node; }
        XMLNode(const FString& name, xmlNodePtr parent = NULL);
        XMLNode(const XMLNode& other) : m_node(other.m_node) { *this = other; }
        XMLNode& operator =(const XMLNode& other) { m_node = other.m_node; return *this; }
        XMLNode& operator =(const xmlNodePtr node) { m_node = node; return *this; }
        virtual ~XMLNode();

    public:
        FString getProp(const char *name) const;
        void setProp(const char *name, const char *value = NULL);
        void delProp(const char *name);
        FString getText() const;
        void nuke();   // detach and delete this node and all of its subchildren

        inline FString getName() const { return m_node == NULL ? "" : FString((const char*)m_node->name); }
        inline xmlNodePtr getChildren() const { return m_node == NULL ? NULL : m_node->children; }
        inline xmlNodePtr getNext() const { return m_node == NULL ? NULL : m_node->next; }
        inline xmlNodePtr getParent() const { return m_node == NULL ? NULL : m_node->parent; }
        inline operator xmlNodePtr() { return m_node; }
        inline operator const xmlNodePtr() const { return m_node; }

    protected:
        friend class XMLDoc;

        ///
        /// Strip all control characters from a string, even if the src is a MBS (UTF-8 string)
        static void stripControls(FString &dest,   ///< destination string
                                  const char *src  ///< Source string, ASCII or MBS
            );

        xmlNodePtr m_node;
    };
};
#endif
#endif
