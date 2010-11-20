#ifndef __dbc_internal_rep_h__
#define __dbc_internal_rep_h__

//#include "Forte.h"
#include "boost/any.hpp"
#include "boost/shared_ptr.hpp"
#include "AnyPtr.h"

using namespace Forte;

// column options
#define OPT_NONE      0x00
#define OPT_UNSIGNED  0x01
#define OPT_NOT_NULL  0x02
#define OPT_AUTO_INCREMENT 0x04
#define OPT_DEFAULT_NULL   0x08
#define OPT_DEFAULT        0x10
#define OPT_IN_PRIMARY_KEY 0x20

//  table options
#define OPT_NOUPDATE    0x40
#define OPT_NOREPLACE   0x80
#define OPT_NOINSERT   0x100
#define OPT_NOSETSQL   0x200
#define OPT_NOKEYSQL   0x400
#define OPT_NOWHERESQL 0x800
#define OPT_NOCTOR    0x1000
#define OPT_NODELETE  0x2000
#define OPT_CUSTOM    0x4000
#define OPT_CUSTOMSET 0x8000

// foreign key options
#define FK_OPT_NONE            0x00
#define FK_OPT_PRIMARY         0x01
#define FK_OPT_DELETE_CASCADE  0x02
#define FK_OPT_DELETE_RESTRICT 0x04
#define FK_OPT_UPDATE_CASCADE  0x08
#define FK_OPT_UPDATE_RESTRICT 0x10
#define FK_OPT_UPDATE_DELETE   0x20


#define SELECTOR_LOCK_UPDATE    0x01
#define SELECTOR_LOCK_SHARED    0x02
#define SELECTOR_LOCK_OPTIONAL  0x04


// Type Convert macro (converts a YYSTYPE arg to the given type)
#define TC(type, arg) (*((*arg).PtrCast<type>()))

#define LASTOBJ() ParseContext::get().getLastObject()
#define TLASTOBJ(type) (*((*(ParseContext::get().getLastObject())).PtrCast<type>()))
#define LASTTABLE() (*((*(ParseContext::get().getLastTable())).PtrCast<Table>()))

#define NEWOBJ(obj) ParseContext::get().setLastObject(YYSTYPE(new AnyPtr(new obj)))

#define PC() ParseContext::get()

#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED
typedef boost::shared_ptr<AnyPtr> YYSTYPE;
#endif

using namespace boost;

namespace DBC {

    class Table;
    class Object;
    class ParseContext {
    public:
        ParseContext();
        ~ParseContext();

        static ParseContext & get(void);

        YYSTYPE setLastObject(YYSTYPE obj); // should be called with each new object parsed
        YYSTYPE getLastObject(void) { return mLastObj; }
        YYSTYPE getLastTable(void) { return mLastTable; }
        
        const FString & getOutputPath(void) { return mOutputPath; }
        const FString & getIncludePath(void) { return mIncludePath; }
        const FString & getFilenamePrefix(void) { return mFilenamePrefix; }

        void getForeignKeysNamed(const char *name, std::vector<YYSTYPE> &keys);

        void setOutputPath(const char *path) { mOutputPath = path; }
        void setIncludePath(const char *path) { mIncludePath = path; }
        void setFilenamePrefix(const char *prefix) { mFilenamePrefix = prefix; }
        void setMakefileName(const char *path) { mMakefileName = path; }

        // validation
        void validate(void);

        // code generation
        void generateCPP(void);
        void generateMk(void);

        YYSTYPE mLastObj;
        YYSTYPE mLastTable;
        std::vector<YYSTYPE> mTables;
        
        std::vector<YYSTYPE> mForeignKeys;
        
        FString mOutputPath;
        FString mIncludePath;
        FString mFilenamePrefix;
        FString mMakefileName;
        static ParseContext *sSingletonPtr;
    };

    class Object {
    public:
        Object() : mOptions(OPT_NONE) {};
        Object(const char *name, int options = OPT_NONE) : 
            mName(name), mOptions(options) {};
        void addOptions(int options) { mOptions |= options; }
        FString mName;
        int mOptions;
    };


    class String : public Object {
    public:
        String(const char *str) : mString(str) {};

        FString mString;
    };
    class ColumnString : public String {
    public:
        ColumnString(const char *str) : String(str) { }
    };
    class IntNum : public String {
    public:
        IntNum(const char *str) : String(str) { }
        int asInt(void) const { return strtol(mString, NULL, 0); }
    };
    class HexNum : public String {
    public:
        HexNum(const char *str) : String(str) { }
    };
    class NullType : public Object {
    public:
        NullType() {};
    };

    class ColumnPredicate {
    public:
        // op is the operator, a and b are o
        ColumnPredicate(unsigned int op, YYSTYPE a, YYSTYPE b) {};
    };

    class ColumnValue {
        // store a column name and an optional value (for Foreign Keys)
    public:
        ColumnValue(const char *column, YYSTYPE value) :
            mColumnName(column), mValue(value), mValueSet(false)
            { if (mValue->GetType() != typeid(NullType)) mValueSet = true; }
        void linkToColumn(const Table &table);
        FString sqlValue(void) const;
        FString mColumnName;
        YYSTYPE mValue;
        bool mValueSet;
        YYSTYPE mColumn;
    };

    class IntType : public Object {
    public:
        IntType(int size, int options = OPT_NONE) : 
            mSize(size) 
            { addOptions(options); }
        
        int mSize;
    };
    class TinyIntType : public Object {
    public:
        TinyIntType(int size) : mSize(size) { };

        int mSize;
    };
    class BigIntType : public Object {
    public:
        BigIntType(int size, int options = OPT_NONE) : mSize(size)
            { addOptions(options); }

        int mSize;
    };
    class BooleanType : public Object {
    public:
        BooleanType() { };
    };

    // STRING TYPES
    class BlobType : public Object {
    public:
        BlobType() { };
    };
    class TextType : public Object {
    public:
        TextType() {};
    };
    class LongTextType : public Object {
    public:
        LongTextType() {};
    };
    class VarCharType : public Object {
    public:
        VarCharType(int size) : mSize(size) { };

        int mSize;
    };
    class CharType : public Object {
    public:
        CharType(int size) : mSize(size) { };

        int mSize;
    };

    class TableColumn : public Object {
    public:
        TableColumn(const char *name, YYSTYPE type, int options) : 
            Object(name), mType(type) { addOptions(options); }

        // given a varchar column called instance_guid, the following methods return:

        // mName (member variable)                              instance_guid
        FString cDecl(void) const;                           // FString mInstanceGUID;
        FString cOriginalDecl(void) const;                   // FString dbc_original_mInstanceGUID;
        FString cTypeName(void) const;                       // FString
        bool cTypeNeedsZeroInit(void) const;                 // false
        FString cPODType(void) const;                        // const char *
        FString cGetter(void) const;                         // GetString
        FString cName(void) const;                           // mInstanceGUID
        FString cOriginalName(void) const;                   // dbc_original_mInstanceGUID
        FString cFormatElement(void) const;                  // '%s'
        FString cFormatStr(const char *aliasPrefix="") const;// instance_guid = '%s'
        FString cFormatParam(const FString &name) const;     // DbEscape(name).c_str()
        FString sqlName(void) const;                         // instance_guid (will quote `key`)
        YYSTYPE mType;
    };

    class TableColumnAccessor : public Object {
    public:
        TableColumnAccessor(const char *name, YYSTYPE colnames);
        /// once the table has been created, linkToColumns() will find the actual TableColumn
        /// objects from the given table and reference them.
        void linkToColumns(const Table &table);

        std::vector<FString> mColumnNames;
        std::vector<YYSTYPE> mColumns;
    };
    class TableConstraint : public TableColumnAccessor {
    public:
        TableConstraint(const char *name, YYSTYPE colnames) :
            TableColumnAccessor(name, colnames) {};
        int mFlags;
    };
    class PrimaryKey : public TableConstraint {
    public:
        PrimaryKey(const char *name, YYSTYPE cols) : TableConstraint(name, cols) {};
            void linkToColumns(const Table &table); // override the base class
    };
    class ForeignKey : public Object {
        // NOTE:  This is not a table constraint, as they are defined within the table
        //        definition itself (builtin MySQL keys, etc).  This is dbc specific.
    public:
        ForeignKey(const char *name, YYSTYPE colvalues, YYSTYPE flags);
        bool columnNameHasValue(const char *name);
        bool compare(const ForeignKey &other);
        FString paramStr(const ForeignKey &callee, bool orig=true) const;
        FString origParamStr(const ForeignKey &callee) const;
        FString localParamStr(const char *prefix = "") const;
        std::vector<YYSTYPE> mColValues;
        YYSTYPE mTable; // table which this foreign key instance is attached to
                        // NOTE: each table also has an array of its foreign keys
        int mFlags;
        int mID;
    };


    class TableCount : public TableColumnAccessor {
    public:
        TableCount(const char *name, YYSTYPE colnames) :
            TableColumnAccessor(name, colnames) {};
        void generateCPP(const Table &t, FILE *h, FILE *c);
        int mFlags;
    };
    class TableSelector : public TableColumnAccessor {
    public:
        TableSelector(const char *name, YYSTYPE colnames, YYSTYPE flags) :
            TableColumnAccessor(name, colnames) {mFlags=TC(int,flags);}
        void generateCPP(const Table &t, FILE *h, FILE *c);
        int mFlags;
    };
    class TableDeletor : public TableColumnAccessor {
    public:
        TableDeletor(const char *name, YYSTYPE colnames) :
            TableColumnAccessor(name, colnames) {};
        void generateCPP(const Table &t, FILE *h, FILE *c);
        int mFlags;
    };
    class TableExistor : public TableColumnAccessor {
    public:
        TableExistor(const char *name, YYSTYPE colnames) :
            TableColumnAccessor(name, colnames) {};
        void generateCPP(const Table &t, FILE *h, FILE *c);
        int mFlags;
    };
    class TableLookup : public TableColumnAccessor {
    public:
        TableLookup(const char *name, YYSTYPE colnames, YYSTYPE retcol) :
            TableColumnAccessor(name, colnames)
        { mRetColName = TC(FString, retcol); }
        void generateCPP(const Table &t, FILE *h, FILE *c);
        int mFlags;
        FString mRetColName;
    };
    class TableCtor : public TableColumnAccessor {
    public:
        TableCtor(YYSTYPE colnames) :
            TableColumnAccessor("", colnames) {};
        void generateCPP(const Table &t, FILE *h, FILE *c);
        int mFlags;
    };
    class Table : public Object {
    public:
        Table(const char *name, YYSTYPE elements);
        ~Table();
        Table(const Table &other);

        void setOutputFilename(const char *filenameBase)
        { mFilenameBase = filenameBase; /* XXX add conflict checking */ }
        void setOutputClassname(const char *classname) 
        { mClassname = classname; /* XXX add conflict checking */ }
        void setAlias(const char *alias)
        { mAlias = alias; }
        void addInclude(const FString &file)
        { mIncludes.push_back(file); }
        inline FString aliasPrefix(void) const
            { 
                if (mAlias.empty()) 
                    return ""; 
                else
                    return FString(FStringFC(),"%s.",mAlias.c_str());
            }
        void addDefine(const char *str) { mDefines.push_back(FString(str)); }

        void addSelector(YYSTYPE name, YYSTYPE colnames, YYSTYPE flags)
            {mSelectors.push_back(YYSTYPE(new AnyPtr(
                                              new TableSelector(TC(FString,name), colnames, flags))));}
        void addDeletor(YYSTYPE name, YYSTYPE colnames) {mDeletors.push_back(YYSTYPE(new AnyPtr(new TableDeletor(TC(FString,name), colnames))));}
        void addExistor(YYSTYPE name, YYSTYPE colnames) {mExistors.push_back(YYSTYPE(new AnyPtr(new TableExistor(TC(FString,name), colnames))));}
        void addLookup(YYSTYPE name, YYSTYPE colnames, YYSTYPE retcol) {mLookups.push_back(YYSTYPE(new AnyPtr(new TableLookup(TC(FString, name), colnames, retcol))));}
        void addCtor(YYSTYPE colnames) {mCtors.push_back(YYSTYPE(new AnyPtr(new TableCtor(colnames))));}
        void addForeignKey(YYSTYPE name, YYSTYPE colvalues, YYSTYPE flags);
        void addCount(YYSTYPE name, YYSTYPE colvalues)
        {mCounts.push_back(YYSTYPE(new AnyPtr(new TableCount(TC(FString,name), colvalues))));}
        
        // code generation:
        void generateCode(void) const;
        void includeCustom(FILE *h, FILE *c) const;
        void cAutogenComment(FILE *o) const;
        void genCtor(FILE *h, FILE *c) const;
        void genRowCtor(FILE *h, FILE *c) const;
        void genPrimaryCtor(FILE *h, FILE *c) const;
        void genDtor(FILE *h, FILE *c) const;
        void genSet(FILE *h, FILE *c) const;
        void genSetOrig(FILE *h, FILE *c) const;
        void genCount(FILE *h, FILE *c) const;
        void genSelect(FILE *h, FILE *c) const;
        void genSelectAlias(FILE *h, FILE *c) const;
        void genSelectPrimaryKey(FILE *h, FILE *c) const;
        void genCounts(FILE *h, FILE *c) const;
        void genSelectors(FILE *h, FILE *c) const;
        void genDeletors(FILE *h, FILE *c) const;
        void genExistors(FILE *h, FILE *c) const;
        void genLookups(FILE *h, FILE *c) const;
        void genCtors(FILE *h, FILE *c) const;
        void genReplace(FILE *h, FILE *c, bool key, bool set) const;
        void genInsert(FILE *h, FILE *c, bool key, bool set) const;
        void genUpdate(FILE *h, FILE *c, bool set, bool where) const;
        void genUpdateKey(FILE *h, FILE *c, bool key, bool set, bool where) const;
        void genDeleteMe(FILE *h, FILE *c, bool where) const;
        void genRefresh(FILE *h, FILE *c, bool where) const;

        void genFKInclude(FILE *c) const;
        void genFKMethods(FILE *h, FILE *c) const;
        void callFKRestrict(FILE *c, bool update) const;
        void genFKRestrict(FILE *h, FILE *c) const;
        void genFKUpdate(FILE *h, FILE *c) const;
        void genFKDelete(FILE *h, FILE *c) const;

        bool genSetSql(FILE *h, FILE *c) const;
        bool genKeySql(FILE *h, FILE *c) const;
        bool genKeyNoAutoSql(FILE *h, FILE *c) const;
        bool genWhereSql(FILE *h, FILE *c) const;

        bool genSetColumnSql(FILE *h, FILE *c) const;
        bool genSetValueSql(FILE *h, FILE *c) const;
        bool genKeyColumnSql(FILE *h, FILE *c) const;
        bool genKeyValueSql(FILE *h, FILE *c) const;
        bool genKeyNoAutoColumnSql(FILE *h, FILE *c) const;
        bool genKeyNoAutoValueSql(FILE *h, FILE *c) const;

        YYSTYPE getColumn(const char *colname) const;

        FString mFilenameBase;
        FString mClassname;
        FString mAlias;
        std::vector<FString> mIncludes;
        std::vector<FString> mDefines;
        std::vector<YYSTYPE> mCounts;
        std::vector<YYSTYPE> mSelectors;
        std::vector<YYSTYPE> mDeletors;
        std::vector<YYSTYPE> mExistors;
        std::vector<YYSTYPE> mLookups;
        std::vector<YYSTYPE> mCtors;
        std::vector<YYSTYPE> mColumns;
        YYSTYPE mAutoIncrementColumn;
        YYSTYPE mCreatedColumn;
        YYSTYPE mModifiedColumn;
        std::vector<YYSTYPE> mConstraints;
        std::vector<YYSTYPE> mForeignKeys;
    };


}; // namespace DBC

#endif
