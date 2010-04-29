#ifndef __DbResult__h
#define __DbResult__h

#ifndef FORTE_NO_DB

#include "FString.h"
#include "DbRow.h"

namespace Forte
{
    class DbResult : public Object
    {
    protected:
        // types
        class Data
        {
        public:
            // ctor/dtor
            Data() { mCount = 0; }
            virtual ~Data() { }

            // abstract interface
            virtual bool isOkay() const = 0;
            virtual bool fetchRow(DbResultRow& row /*OUT*/) = 0;
            virtual size_t getNumColumns() = 0;
            virtual FString getColumnName(size_t i) = 0;
            virtual size_t getFieldLength(size_t i) = 0;
            virtual size_t getNumRows() = 0;
            virtual bool seek(size_t offset) = 0;

            // extra convenience method
            virtual bool fetchRow(DbRow& row /*OUT*/);

            // memory management (use within a single thread only)
            void AddRef() { mCount++; };
            void Release() { if (--mCount == 0) delete this; };
        private:
            int mCount;
        };

    public:
        // ctor/dtor
        DbResult();
        DbResult(const DbResult& other) { mData = NULL; *this = other; }
        DbResult(Data *data) { mData = NULL; *this = data; }
        virtual ~DbResult() { Clear(); }

        // copy operators
        DbResult& operator =(const DbResult& other);
        DbResult& operator =(Data *data);

        // helpers
        void Clear();

        // delegated members, a.k.a. the interface
        inline operator bool() const  { return (mData != NULL) && mData->isOkay(); }
        inline bool operator !() const { return (mData == NULL) || !mData->isOkay(); }
        inline bool isOkay() { return (mData != NULL) && mData->isOkay(); }
        inline bool fetchRow(DbResultRow& row) { return (mData != NULL) && mData->fetchRow(row); }
        inline bool fetchRow(DbRow& row) { return (mData != NULL) && mData->fetchRow(row); }
        inline size_t getNumColumns() { return (mData ? mData->getNumColumns() : 0); }
        inline FString getColumnName(size_t i) { return (mData ? mData->getNumColumns() : 0); }
        inline size_t getFieldLength(size_t i) { return (mData ? mData->getFieldLength(i) : 0); }
        inline size_t getNumRows() { return (mData ? mData->getNumRows() : 0); }
        inline bool seek(size_t offset) { return (mData ? mData->seek(offset) : false); }
    protected:
        Data *mData;
    };
};
#endif
#endif
