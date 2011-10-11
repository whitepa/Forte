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
            virtual bool IsOkay() const = 0;
            virtual bool FetchRow(DbResultRow& row /*OUT*/) = 0;
            virtual void UnFetchRow() = 0;
            virtual size_t GetNumColumns() = 0;
            virtual FString GetColumnName(size_t i) = 0;
            virtual size_t GetFieldLength(size_t i) = 0;
            virtual size_t GetNumRows() = 0;
            virtual bool Seek(size_t offset) = 0;

            // extra convenience method
            virtual bool FetchRow(DbRow& row /*OUT*/);

            // memory management (use within a single thread only)
            virtual void AddRef() { mCount++; };
            virtual void Release() { if (--mCount == 0) delete this; };
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
        inline operator bool() const  { return (mData != NULL) && mData->IsOkay(); }
        inline bool operator !() const { return (mData == NULL) || !mData->IsOkay(); }
        inline bool IsOkay() { return (mData != NULL) && mData->IsOkay(); }
        inline bool FetchRow(DbResultRow& row) { return (mData != NULL) && mData->FetchRow(row); }
        inline bool FetchRow(DbRow& row) { return (mData != NULL) && mData->FetchRow(row); }
        inline void UnFetchRow() { if (mData) mData->UnFetchRow(); }
        inline size_t GetNumColumns() { return (mData ? mData->GetNumColumns() : 0); }
        inline FString GetColumnName(size_t i) { return (mData ? mData->GetNumColumns() : 0); }
        inline size_t GetFieldLength(size_t i) { return (mData ? mData->GetFieldLength(i) : 0); }
        inline size_t GetNumRows() { return (mData ? mData->GetNumRows() : 0); }
        inline bool Seek(size_t offset) { return (mData ? mData->Seek(offset) : false); }
     protected:
        Data *mData;
    };
};
#endif
#endif
