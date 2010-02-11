#ifndef __DbResult__h
#define __DbResult__h

#ifndef FORTE_NO_DB

#include "FString.h"
#include "DbRow.h"

namespace Forte
{
    class CDbResult
    {
    protected:
        // types
        class Data
        {
        public:
            // ctor/dtor
            Data() { m_count = 0; }
            virtual ~Data() { }

            // abstract interface
            virtual bool isOkay() const = 0;
            virtual bool fetchRow(CDbResultRow& row /*OUT*/) = 0;
            virtual size_t getNumColumns() = 0;
            virtual FString getColumnName(size_t i) = 0;
            virtual size_t getFieldLength(size_t i) = 0;
            virtual size_t getNumRows() = 0;
            virtual bool seek(size_t offset) = 0;

            // extra convenience method
            virtual bool fetchRow(CDbRow& row /*OUT*/);

            // memory management (use within a single thread only)
            void AddRef() { m_count++; };
            void Release() { if (--m_count == 0) delete this; };
        private:
            int m_count;
        };

    public:
        // ctor/dtor
        CDbResult();
        CDbResult(const CDbResult& other) { m_data = NULL; *this = other; }
        CDbResult(Data *data) { m_data = NULL; *this = data; }
        virtual ~CDbResult() { Clear(); }

        // copy operators
        CDbResult& operator =(const CDbResult& other);
        CDbResult& operator =(Data *data);

        // helpers
        void Clear();

        // delegated members, a.k.a. the interface
        inline operator bool() const  { return (m_data != NULL) && m_data->isOkay(); }
        inline bool operator !() const { return (m_data == NULL) || !m_data->isOkay(); }
        inline bool isOkay() { return (m_data != NULL) && m_data->isOkay(); }
        inline bool fetchRow(CDbResultRow& row) { return (m_data != NULL) && m_data->fetchRow(row); }
        inline bool fetchRow(CDbRow& row) { return (m_data != NULL) && m_data->fetchRow(row); }
        inline size_t getNumColumns() { return (m_data ? m_data->getNumColumns() : 0); }
        inline FString getColumnName(size_t i) { return (m_data ? m_data->getNumColumns() : 0); }
        inline size_t getFieldLength(size_t i) { return (m_data ? m_data->getFieldLength(i) : 0); }
        inline size_t getNumRows() { return (m_data ? m_data->getNumRows() : 0); }
        inline bool seek(size_t offset) { return (m_data ? m_data->seek(offset) : false); }
    protected:
        Data *m_data;
    };
};
#endif
#endif
