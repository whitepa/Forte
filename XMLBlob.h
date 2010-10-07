#ifndef __forte_XMLBlob_h
#define __forte_XMLBlob_h

#ifndef FORTE_NO_XML

#include "FString.h"
#include "Exception.h"
#include <libxml/parser.h>

#if !defined(LIBXML_TREE_ENABLED) || !defined(LIBXML_OUTPUT_ENABLED)
#error libxml not configured with tree and output support
#endif

/// XMLBlob is a fairly simple, free-form, non-validating, XML writing class.  The XML document
/// must be constructed from beginning to end.
///
namespace Forte
{
    EXCEPTION_SUBCLASS(Exception, EXMLBlobStripControlsFailed);

    class XMLBlob
    {
    public:
        /// Construct a new XML document with the specified root node name.
        ///
        XMLBlob(const char *rootName ///< The name of the document's root element.
            );

        /// Future plans include parsing of XML documents, which this constructor would be used for.
        /// Currently, this constructor will throw an exception, as it is not yet implemented.
        ///
        XMLBlob(const FString &in);

        /// The XMLBlob destructor frees all internally allocated memory.
        ///
        virtual ~XMLBlob();
    
        /// Create a child element of the previously added node.  This effectively 'pushes' an element 
        /// on to a stack, such that all data nodes and child nodes added after this call will end up
        /// as children of this new node.
        ///
        void BeginChild(const char *name  ///< name of the new element
            );

        /// This method closes the most recently added child element, effectively 'popping' it from the stack.
        /// Any elements added after this call will be added as children of the next highest ancestor.
        ///
        void EndChild(void);

        inline xmlNodePtr GetCurrentNode(void) { return current; }

        /// Add an attribute to the previously added child node.
        ///
        void AddAttribute(const char *name, const char *value);

        /// Add an attribute to the previously added data node (non-child node).
        ///
        void AddDataAttribute(const char *name, const char *value);

        /// Add a data node as a child of the most recently added child node.
        ///
        void AddData(const char *name, const char *value);
        void AddDataRaw(const char *name, const char *value);

        ///
        /// Add a data node as a child of the most recently added child node.
        inline void AddData(const char *name, const std::string &value) { AddData(name, value.c_str()); }
        inline void AddDataRaw(const char *name, const std::string &value) { AddDataRaw(name, value.c_str()); }
        ///
        /// Add a data node as a child of the most recently added child node.
        inline void AddData(const char *name, unsigned int value) { AddData(name, FString(value)); }
        inline void AddDataRaw(const char *name, unsigned int value) { AddDataRaw(name, FString(value)); }
        ///
        /// Add a data node as a child of the most recently added child node.
        inline void AddData(const char *name, int value) { AddData(name, FString(value)); }
        inline void AddDataRaw(const char *name, int value) { AddDataRaw(name, FString(value)); }
        ///
        /// Add a data node as a child of a previously added node.  This method is the only 
        /// method in the  class which allows some kind of random access to the XML structure.  
        void AddDataToNode(xmlNodePtr node, const char *name, const char *value);
        void AddDataToNodeRaw(xmlNodePtr node, const char *name, const char *value);

        ///
        /// Output the current XML document into an FString, optionally indenting it for convenience.
        void ToString(FString &out,       ///< output XML
                      bool pretty = false ///< If true, output will be properly indented.
            );

    protected:
        ///
        /// Strip all control characters from a string, even if the src is a MBS (UTF-8 string)
        static void stripControls(FString &dest,   ///< destination string
                                  const char *src  ///< Source string, ASCII or MBS
            );

        const bool readOnly; ///< read only flag, not currently used as we don't parse yet

        xmlDocPtr doc;       ///< the XML document
        xmlNodePtr root;     ///< pointer to the document's root node
        xmlNodePtr current;  ///< pointer to the current node (any node which has just been created)
        xmlNodePtr lastData; ///< pointer to the last data node which was created
    };
};
#endif // FORTE_NO_XML
#endif // __forte_XMLBlob_h
