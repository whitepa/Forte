#include "InternalRep.h"
#include "Foreach.h"
#include <iostream>

using namespace DBC;
using namespace std;

ParseContext * DBC::ParseContext::sSingletonPtr = NULL;

ParseContext::ParseContext()
{
    sSingletonPtr = this;
    mOutputPath = "dblayer";
    mIncludePath = "include";
//    cout << "Created parse context" << endl;
}

ParseContext & ParseContext::get(void)
{
    if (!sSingletonPtr) throw Exception("invalid parse context");
    return *sSingletonPtr;
}

YYSTYPE ParseContext::setLastObject(YYSTYPE obj)
{
    mLastObj = obj;
//     cout << "last object is a ";
    if ((*obj).GetType() == typeid(Table))
    {
        mLastTable = obj;
// cout << " table ";
    }
//     else if ((*obj).GetType() == typeid(TableColumn)) cout << " table column ";
//     else cout << " other ";
//     cout << endl;
    return obj;
}

void ParseContext::getForeignKeysNamed(const char *name, std::vector<YYSTYPE> &keys)
{
    keys.clear();
    foreach(YYSTYPE k, mForeignKeys)
        if (!TC(ForeignKey,k).mName.compare(name))
            keys.push_back(k);
}

void ParseContext::validate(void)
{
//    cout << "Foreign Keys Defined:" << endl;
//    foreach (YYSTYPE a, mForeignKeys)
//    {
//        const ForeignKey &fk(TC(ForeignKey,a));
//        cout << TC(Table,fk.mTable).mName << " :: " << fk.mName << endl;
//    }
    // XXX make sure all foreign keys have one and only one primary
    
    
}

void ParseContext::generateCPP(void)
{
    int count = 0;
    foreach (YYSTYPE p, mTables)
    {
        Table &t(TC(Table,p));
        t.generateCode();
        ++count;
    }
    cout << "Generated code for " << count << " tables." << endl;
}
void ParseContext::generateMk(void)
{
    FILE *m;
    FString mFilename;
    if (!mMakefileName.empty())
        mFilename.Format("%s/%s", 
                         ParseContext::get().getOutputPath().c_str(),
                         mMakefileName.c_str());
    else
        mFilename.Format("%s/dbc_generated_srcs.mk", 
                         ParseContext::get().getOutputPath().c_str());
    if ((m = fopen(mFilename, "w"))==NULL)
        throw Exception(FStringFC(), "unable to open file '%s'", mFilename.c_str());

    fprintf(m, "#\n");
    fprintf(m, "# THIS FILE WAS AUTOMATICALLY GENERATED; DO NOT EDIT!\n");
    fprintf(m, "#\n\n");
    fprintf(m, "DBC_GENERATED_SRCS = ");
    foreach (YYSTYPE p, mTables)
    {
        Table &t(TC(Table,p));
        fprintf(m, " \\\n\t%s%s.cpp", PC().getFilenamePrefix().c_str(), t.mFilenameBase.c_str());
    }
    fprintf(m, "\n\n");
    fclose(m);
}

ParseContext::~ParseContext()
{
    sSingletonPtr = NULL;
}

YYSTYPE Table::getColumn(const char *colname) const
{
    YYSTYPE ret;
    bool found = false;
    foreach (YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (!tc.mName.Compare(colname))
        {
            // found it
            ret = a;
            found = true;
            break;
        }
    }
    if (!found) throw Exception(FStringFC(), "Unknown column %s in Foreign Key for table '%s'",
                                 colname, mName.c_str());
    return ret;
}

void ColumnValue::linkToColumn(const Table &table)
{
    bool found = false;
    // find this column among the table's columns
    foreach (YYSTYPE a, table.mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (!tc.mName.Compare(mColumnName))
        {
            // found it
            mColumn = a;
            found = true;
            break;
        }
    }
    if (!found) throw Exception(FStringFC(), "Unknown column %s in Foreign Key for table '%s'",
                                 mColumnName.c_str(), table.mName.c_str());
}

FString ColumnValue::sqlValue(void) const
{
    if (mValue->GetType() == typeid(IntNum))
    {
        const IntNum &i(TC(IntNum,mValue));
        return i.mString;
    }
    else if (mValue->GetType() == typeid(FString))
    {
        const FString &i(TC(FString,mValue));
        return FString(FStringFC(), "'%s'",i.c_str());
    }
    else
        throw Exception("unsupported type in foreign key value");
}

ForeignKey::ForeignKey(const char *name, YYSTYPE colvalues, YYSTYPE flags) :
    Object(name), mID(0)
{
    mFlags = TC(int, flags);
    mTable = ParseContext::get().getLastTable();
//    cout << "FOREIGN KEY " << name << " WITH FLAGS " << mFlags << endl;
    // validate this foreign key
    // if this is the primary table, we cannot have a value set
    std::vector<YYSTYPE> many(TC(std::vector<YYSTYPE>,colvalues));
    foreach (YYSTYPE a, many)
    {
        if (a->GetType() == typeid(ColumnValue))
        {
            ColumnValue &cv(TC(ColumnValue,a));
            if (mFlags & FK_OPT_PRIMARY) 
            {
                if (cv.mValueSet)
                    throw Exception("Values must not be set in foreign key references"
                                     " for primary tables");
                
            }
            cv.linkToColumn(LASTTABLE());
            mColValues.push_back(a);
        }
    }
    
    // insert this Foreign Key into the global structure
    // first, check all other table references of this foreign key
    foreach (YYSTYPE p, PC().mForeignKeys)
    {
        ++mID;
        ForeignKey &fk(TC(ForeignKey,p));
        // compare this foreign key to the global foreign key
        if (!fk.mName.compare(mName))
        {
            // names match, make sure the types of the non-valued fields match up in order
            if (compare(fk))
                throw Exception(FStringFC(),
                                 "Foreign Key '%s' on table '%s' does not match previous definitions.",
                                 name, LASTTABLE().mName.c_str());
        }
    }
    // add to the list
}
bool ForeignKey::columnNameHasValue(const char *name)
{
    // loop through the column values and see if there is a match with a value
    foreach (YYSTYPE a, mColValues)
    {
        const ColumnValue &cv(TC(ColumnValue,a));
        if (!cv.mColumnName.compare(name))
            return cv.mValueSet;
    }
    return false;
}
bool ForeignKey::compare(const ForeignKey &other)
{
    // check if the foreign keys are compatible:
    // return true if they are not compatible

    std::vector<YYSTYPE>::const_iterator i2;
    i2 = other.mColValues.begin();
    foreach(YYSTYPE a, mColValues)
    {
        const ColumnValue &c1(TC(ColumnValue,a));
        // if value set on this, go to the next
        if (c1.mValueSet) continue;
        // if value set on other, find the next non-value
        while (i2 != other.mColValues.end() && TC(ColumnValue,*i2).mValueSet) ++i2;
        if (i2 != other.mColValues.end())
        {
            if (TC(TableColumn,(TC(ColumnValue,*i2).mColumn)).cTypeName().compare(
                    TC(TableColumn,c1.mColumn).cTypeName()))
            {
                // types are different
                cout << "type mismatch" << endl;
                return true;
            }
            ++i2;
        }
        else // more columns in c1
        {
            cout << "extra columns in foreign key(1) '" << mName << "' on table " << TC(Table,mTable).mName << endl;
            return true;
        }
    }
    while (i2 != other.mColValues.end())
    {
        if (!TC(ColumnValue,*i2).mValueSet)
        {
            // more columns in c2
            cout << "extra columns in foreign key(2) '" << other.mName << "' on table " << TC(Table,other.mTable).mName << endl;
            return true;
        }
        ++i2;
    }
    // looks good
    return false;
}
FString ForeignKey::paramStr(const ForeignKey &callee, bool orig) const
{
    // return a parameter string containing this foreign key's columns
    // which match up with the callee table's foreign key columns
    std::vector<FString> params;
    if (!(mFlags & FK_OPT_PRIMARY))
        throw Exception("Only a primary foreign key instance can call others");
    std::vector<YYSTYPE>::const_iterator i2;
    i2 = callee.mColValues.begin();
    foreach(YYSTYPE a, mColValues)
    {
        const ColumnValue &c1(TC(ColumnValue,a));
        // if value set on callee, find the next non-value
        while (i2 != callee.mColValues.end() && TC(ColumnValue,*i2).mValueSet) ++i2;
        if (i2 != callee.mColValues.end())
        {
            const TableColumn &thisColumn(TC(TableColumn,c1.mColumn));
            const TableColumn &otherColumn(TC(TableColumn,(TC(ColumnValue,*i2).mColumn)));
            if (otherColumn.cTypeName().compare(thisColumn.cTypeName()))
                // types are different
                throw Exception("foreign key type mismatch");
            if (orig)
                params.push_back(thisColumn.cOriginalName()); // using original name
            else
                params.push_back(thisColumn.cName()); // using original name                
            ++i2;
        }
        else // more columns in c1
            throw Exception(FStringFC(), "extra columns in foreign key(1) '%s' on table '%s'",
                             mName.c_str(),TC(Table,mTable).mName.c_str());
    }
    while (i2 != callee.mColValues.end())
    {
        if (!TC(ColumnValue,*i2).mValueSet)
            // more columns in c2
            throw Exception(FStringFC(), "extra columns in foreign key(2) '%s' on table '%s'",
                             callee.mName.c_str(),TC(Table,callee.mTable).mName.c_str());
        ++i2;
    }
    // looks good
    FString paramStr;
    paramStr.Implode(", ", params);
    return paramStr;
}
FString ForeignKey::localParamStr(const char *prefix) const
{
    // return a parameter string containing this foreign key's non-valued columns
    std::vector<FString> params;
    foreach(YYSTYPE a, mColValues)
    {
        const ColumnValue &c(TC(ColumnValue,a));
        const TableColumn &col(TC(TableColumn,c.mColumn));
        if (c.mValueSet) continue;
        params.push_back(FString(FStringFC(), "%s %s%s", 
                                 col.cPODType().c_str(), prefix, col.mName.c_str()));
    }
    FString paramStr;
    paramStr.Implode(", ", params);
    return paramStr;
}

TableColumnAccessor::TableColumnAccessor(const char *name, YYSTYPE colnames):
    Object(name)
{
    std::vector<YYSTYPE> many(TC(std::vector<YYSTYPE>,colnames));
    foreach(YYSTYPE a, many)
    {
        if (a->GetType() == typeid(FString))
        {
            const FString &s(TC(FString,a));
            mColumnNames.push_back(s);
        }
        else if (a->GetType() == typeid(ColumnValue))
        {
            const FString &s(TC(ColumnValue,a).mColumnName);
            mColumnNames.push_back(s);
        }
        else
        {
            throw Exception("unknown type in new TableConstraint columns");
        }
    }
}
void TableColumnAccessor::linkToColumns(const Table &table)
{
    // loop through the column names
    foreach (FString &colName, mColumnNames)
    {
        bool found = false;
        // find this column among the table's columns
        foreach (YYSTYPE a, table.mColumns)
        {
            TableColumn &tc(TC(TableColumn,a));
            if (!tc.mName.Compare(colName))
            {
                // found it
                mColumns.push_back(a);
                found = true;
                break;
            }
        }
        if (!found) throw Exception(FStringFC(), "Unknown column %s in table column accessor",
                                     colName.c_str());
    }
}

void PrimaryKey::linkToColumns(const Table &table)
{
    // loop through the column names
    foreach (FString &colName, mColumnNames)
    {
        bool found = false;
        // find this column among the table's columns
        foreach (YYSTYPE a, table.mColumns)
        {
            TableColumn &tc(TC(TableColumn,a));
            if (!tc.mName.Compare(colName))
            {
                // found it
                tc.addOptions(OPT_IN_PRIMARY_KEY);
//                cout << " >>> FOUND PRIMARY KEY COLUMN " << colName << endl;
                mColumns.push_back(a);
                found = true;
                break;
            }
        }
        if (!found) throw Exception(FStringFC(), "Unknown column %s in primary key",
                                     colName.c_str());
    }
}


void TableCount::generateCPP(const Table &t, FILE *h, FILE *c)
{
    // generate the SQL format string and the C format params
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    foreach (YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        formatStrV.push_back(tc.cFormatStr(t.aliasPrefix()));
        formatParamV.push_back(tc.cFormatParam(tc.mName));
    }
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        keyParamV.push_back(FString(FStringFC(), 
                                    "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
    }
    keyParamStr.Implode(", ", keyParamV);

    fprintf(h, "    static unsigned int %s(Forte::DbConnection &db, %s);\n", mName.c_str(), keyParamStr.c_str());
    fprintf(c, "unsigned int %s::%s(Forte::DbConnection &db, %s)\n", 
            t.mClassname.c_str(), mName.c_str(), keyParamStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" WHERE %s \",\n",formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    return Count(db, sql);\n");
    fprintf(c, "}\n");
}

void TableSelector::generateCPP(const Table &t, FILE *h, FILE *c)
{
    // generate the SQL format string and the C format params
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    foreach (YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        formatStrV.push_back(tc.cFormatStr(t.aliasPrefix()));
        formatParamV.push_back(tc.cFormatParam(tc.mName));
    }
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        keyParamV.push_back(FString(FStringFC(), 
                                    "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
    }
    // if locking was specified, generate the lock string
    FString lockDefault;
    if (mFlags & SELECTOR_LOCK_OPTIONAL)
    {
        keyParamV.push_back(FString("unsigned int lockMode"));
        lockDefault = " = REV_DB_NO_LOCK";
    }
    keyParamStr.Implode(", ", keyParamV);

    FString lockStr;
    if (mFlags & SELECTOR_LOCK_UPDATE)
        lockStr = "FOR UPDATE";
    else if (mFlags & SELECTOR_LOCK_SHARED)
        lockStr = "LOCK IN SHARE MODE";
    fprintf(h, "    static Forte::DbResult %s(Forte::DbConnection &db, %s%s);\n", mName.c_str(), keyParamStr.c_str(), lockDefault.c_str());
    fprintf(c, "Forte::DbResult %s::%s(Forte::DbConnection &db, %s)\n", 
            t.mClassname.c_str(), mName.c_str(), keyParamStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" WHERE %s %s \",\n",formatStr.c_str(),
            (mFlags & SELECTOR_LOCK_OPTIONAL) ? "" : lockStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    if (mFlags & SELECTOR_LOCK_OPTIONAL)
    {
        fprintf(c, "    if (lockMode == REV_DB_LOCK_UPDATE)\n");
        fprintf(c, "        sql.append(\" FOR UPDATE \");\n");
        fprintf(c, "    else if (lockMode == REV_DB_LOCK_SHARED)\n");
        fprintf(c, "        sql.append(\" LOCK IN SHARE MODE \");\n");
    }
    fprintf(c, "    return Select(db, sql);\n");
    fprintf(c, "}\n");
}


void TableDeletor::generateCPP(const Table &t, FILE *h, FILE *c)
{
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    foreach (YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        formatStrV.push_back(tc.cFormatStr());
        formatParamV.push_back(tc.cFormatParam(tc.mName));
    }
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        keyParamV.push_back(FString(FStringFC(), 
                                    "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
    }
    keyParamStr.Implode(", ", keyParamV);

    fprintf(h, "    static void %s(Forte::DbConnection &db, %s);\n", mName.c_str(), keyParamStr.c_str());
    fprintf(c, "void %s::%s(Forte::DbConnection &db, %s)\n", 
            t.mClassname.c_str(), mName.c_str(), keyParamStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"DELETE FROM %s WHERE %s \",\n",t.mName.c_str(), formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");
    fprintf(c, "}\n");
}

void TableExistor::generateCPP(const Table &t, FILE *h, FILE *c)
{
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    foreach (YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        formatStrV.push_back(tc.cFormatStr());
        formatParamV.push_back(tc.cFormatParam(tc.mName));
    }
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        keyParamV.push_back(FString(FStringFC(), 
                                    "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
    }
    keyParamStr.Implode(", ", keyParamV);

    fprintf(h, "    static bool %s(Forte::DbConnection &db, %s);\n", mName.c_str(), keyParamStr.c_str());
    fprintf(c, "bool %s::%s(Forte::DbConnection &db, %s)\n", 
            t.mClassname.c_str(), mName.c_str(), keyParamStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"SELECT 1 FROM %s WHERE %s \",\n",t.mName.c_str(), formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    Forte::DbResult res = Forte::DbUtil::DbStore(db, sql);\n");
    fprintf(c, "    return (res.GetNumRows() != 0);\n");
    fprintf(c, "}\n");
}

void TableLookup::generateCPP(const Table &t, FILE *h, FILE *c)
{
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    foreach (YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        formatStrV.push_back(tc.cFormatStr());
        formatParamV.push_back(tc.cFormatParam(tc.mName));
    }
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        keyParamV.push_back(FString(FStringFC(), 
                                    "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
    }
    keyParamStr.Implode(", ", keyParamV);

    // figure out return column
    const TableColumn &retcol(TC(TableColumn,t.getColumn(mRetColName)));

    fprintf(h, "    static %s %s(Forte::DbConnection &db, %s);\n", retcol.cTypeName().c_str(),
            mName.c_str(), keyParamStr.c_str());
    fprintf(c, "%s %s::%s(Forte::DbConnection &db, %s)\n", 
            retcol.cTypeName().c_str(), t.mClassname.c_str(), mName.c_str(), keyParamStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"SELECT %s FROM %s WHERE %s \",\n",
            retcol.sqlName().c_str(), t.mName.c_str(), formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    Forte::DbResult res = Forte::DbUtil::DbStore(db, sql);\n");
    fprintf(c, "    Forte::DbResultRow row;\n");
    fprintf(c, "    if (!res.FetchRow(row))\n");
    fprintf(c, "        throw Forte::DbException(\"Unable to lookup '%s'\");\n", mName.c_str());
    fprintf(c, "    return %s(row, 0);\n", retcol.cGetter().c_str());
    fprintf(c, "}\n");
}

void TableCtor::generateCPP(const Table &t, FILE *h, FILE *c)
{
    // generate the initialization list
    std::vector<FString> initStrV;
    FString initStr;
    // loop through all columns in the table
    foreach (YYSTYPE a, t.mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        bool isParameter = false;
        // see if this column is in our column list as a parameter
        foreach (YYSTYPE b, mColumns)
        {
            if (a==b)
            {
                // the column is a parameter, initialize it to the parameter
                FString init;
                init.Format("%s(%s)",tc.cName().c_str(), tc.mName.c_str());
                initStrV.push_back(init);
                isParameter = true;
                break;
            }
        }
        if (!isParameter && tc.cTypeNeedsZeroInit())
            // the column was not a parameter, and it needs initialization
            initStrV.push_back(FString(FStringFC(), "%s(0)", tc.cName().c_str()));
    }
    initStr.Implode(",\n        ", initStrV);

    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        keyParamV.push_back(FString(FStringFC(), 
                                    "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
    }
    keyParamStr.Implode(", ", keyParamV);

    fprintf(h, "    %s(%s) :\n", t.mClassname.c_str(), keyParamStr.c_str());
    fprintf(h, "        %s\n", initStr.c_str());
    fprintf(h, "    {};\n");
}


Table::~Table()
{
    // delete all elements
//    cout << "Table destructor with filename " << mFilenamePrefix << endl;
}


Table::Table(const char *name, YYSTYPE elements) : 
    Object(name)
{
    mClassname.Format("db_%s", name);
    mFilenameBase = mClassname;

//    cout << ">>> table name " << name << endl;
    std::vector<YYSTYPE> many(TC(std::vector<YYSTYPE>,elements));
//    cout << " any cast complete " << endl;
    foreach(YYSTYPE a, many)
    {
        if (a->GetType() == typeid(TableColumn))
        {
            const TableColumn &tc(TC(TableColumn,a));
//            cout << "  >>> table column name " << tc.mName << endl;
            mColumns.push_back(a);
            // set pointers if 'created' or 'modified'
            if (!tc.mName.Compare("created"))
                mCreatedColumn = a;
            else if (!tc.mName.Compare("modified"))
                mModifiedColumn = a;
            // set pointer if this is the auto_increment column
            if (tc.mOptions & OPT_AUTO_INCREMENT)
                mAutoIncrementColumn = a;
        }
        else if (a->GetType() == typeid(TableConstraint))
        {
//            cout << "  >>> table constraint" << endl;
            TC(TableConstraint,a).linkToColumns(*this); // link the constraint to our columns
            mConstraints.push_back(a);
        }
        else if (a->GetType() == typeid(PrimaryKey))
        {
//            cout << "  >>> primary key " << endl;
            TC(PrimaryKey,a).linkToColumns(*this); // link the constraint to our columns
            mConstraints.push_back(a);
        }
//        else
//            cout << "WARNING: unknown type in Table ctor" << endl;
    }
}

Table::Table(const Table &other)
{
    cout << "WARNING: Copy Constructor shouldn't be called" << endl;
}
void Table::addForeignKey(YYSTYPE name, YYSTYPE colvalues, YYSTYPE flags)
{
    YYSTYPE fk = YYSTYPE(new AnyPtr(new ForeignKey(TC(FString,name), colvalues, flags)));
    // add to the table's list of foreign keys
    mForeignKeys.push_back(fk);
    // add to the global list of foreign keys
    PC().mForeignKeys.push_back(fk);
}
void Table::generateCode(void) const
{
    FILE *h, *c;
    FString filename;
    FString filenameBase = PC().getFilenamePrefix() + mFilenameBase;
    filename.Format("%s/%s.h", 
                    ParseContext::get().getOutputPath().c_str(),
                    filenameBase.c_str());
    cout << "Writing file " << filename << endl;
    if ((h = fopen(filename, "w"))==NULL)
        throw Exception(FStringFC(), "unable to open file '%s'", filename.c_str());
    filename.Format("%s/%s.cpp", 
                    ParseContext::get().getOutputPath().c_str(),
                    filenameBase.c_str());
    cout << "Writing file " << filename << endl;
    if ((c = fopen(filename, "w"))==NULL)
        throw Exception(FStringFC(), "unable to open file '%s'", filename.c_str());
    
    cAutogenComment(h);
    cAutogenComment(c);
    fprintf(h, "#ifndef __dbc__%s_h__\n", filenameBase.c_str());
    fprintf(h, "#define __dbc__%s_h__\n", filenameBase.c_str());
    fprintf(h, "\n");
    fprintf(h, "#include \"Forte.h\"\n");
    foreach(FString file, mIncludes)
        fprintf(h, "#include \"%s\"\n", file.c_str());
    fprintf(h, "\n");
    foreach(FString d, mDefines)
    {
        fprintf(h, "#define %s\n", d.c_str());
    }
    fprintf(h, "\n");
    fprintf(h, "class %s : public Forte::DbRow\n", mClassname.c_str());
    fprintf(h, "{\n");
    fprintf(h, "public:\n");

    fprintf(c, "#include \"%s.h\"\n", filenameBase.c_str());
    genFKInclude(c);
    fprintf(c, "\n");
    if (!(mOptions & OPT_NOCTOR)) genCtor(h,c);
    genRowCtor(h,c);
    genCtors(h,c);
    genPrimaryCtor(h,c);
    genDtor(h,c);
    genSet(h,c);
    genSetOrig(h,c);
    fprintf(h, "\n");
    genCount(h,c);
    genSelect(h,c);
    genSelectAlias(h,c);
    fprintf(h, "\n");
    fprintf(h, "protected:\n");
    bool set,key,keynoauto,where;
    set = genSetSql(h,c);
    if (set)
    {
        genSetColumnSql(h,c);
        genSetValueSql(h,c);
    }
    key = genKeySql(h,c);
    if (key)
    {
        genKeyColumnSql(h,c);
        genKeyValueSql(h,c);
    }
    keynoauto = genKeyNoAutoSql(h,c);
    if (keynoauto)
    {
        genKeyNoAutoColumnSql(h,c);
        genKeyNoAutoValueSql(h,c);
    }
    where = genWhereSql(h,c);
    fprintf(h, "\n");
    fprintf(h, "public:\n");
    genSelectPrimaryKey(h,c);
    genCounts(h,c);
    genSelectors(h,c);
    genDeletors(h,c);
    genExistors(h,c);
    genLookups(h,c);
    if (!(mOptions & OPT_NOREPLACE)) genReplace(h,c,key,set);
    if (!(mOptions & OPT_NOINSERT))  genInsert(h,c,keynoauto,set);
    if (!(mOptions & OPT_NOUPDATE))  genUpdate(h,c,set,where);
    if (!(mOptions & OPT_NOUPDATE))  genUpdateKey(h,c,key,set,where);
    if (!(mOptions & OPT_NODELETE))  genDeleteMe(h,c,where);
    genRefresh(h,c,where);
    genFKRestrict(h,c);
    genFKUpdate(h,c);
    genFKDelete(h,c);
    
    if (mOptions & OPT_CUSTOM)  includeCustom(h,c);
    fprintf(h, "\n");    
    fprintf(h, "public:\n");
    // member variables (columns):
    foreach(YYSTYPE c, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,c));
        fprintf(h, "    %s\n", tc.cDecl().c_str());
    }
    fprintf(h, "protected:\n");
    // store original values of primary key, for the update() call
    fprintf(h, "    bool dbc_original_loaded;\n"); // flag to indicate
                                                   // if original data
                                                   // was loaded from
                                                   // database
    foreach(YYSTYPE c, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,c));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
            fprintf(h, "    %s\n", tc.cOriginalDecl().c_str());
    }
    fprintf(h, "};\n\n");
    fprintf(h, "#endif // __dbc__%s_h__\n", filenameBase.c_str());
    fclose(h);
    fclose(c);
}

void Table::includeCustom(FILE *h, FILE *c) const
{
    FString hFile, cFile;
    ParseContext &pc(ParseContext::get());
    hFile.Format("%s/%s.custom.h", pc.getIncludePath().c_str(), mFilenameBase.c_str());
    cFile.Format("%s/%s.custom.cpp", pc.getIncludePath().c_str(), mFilenameBase.c_str());
//     FString hCode, cCode;
//     try {
//         FString::LoadFile(hFile, 0x1000000, hCode);
//         fprintf(h, "\n// Begin Custom (included from %s)\n\n", hFile.c_str());
//         fprintf(h, "#line 1 \"%s\"\n", hFile.c_str());
//         fwrite(hCode.c_str(), sizeof(char), hCode.size(), h);
//         fprintf(h, "\n// End Custom\n\n");
//     } catch (Exception &e) {
//     }

//     try {
//         FString::LoadFile(cFile, 0x1000000, cCode);
//         fprintf(c, "\n// Begin Custom (included from %s)\n\n", cFile.c_str());
//         fprintf(c, "#line 1 \"%s\"\n", cFile.c_str());
//         fwrite(cCode.c_str(), sizeof(char), cCode.size(), c);
//         fprintf(c, "\n// End Custom\n\n");
//     } catch (Exception &e) {
//     }
    // instead, just make a #include; line numbers and dependencies should just work
    fprintf(h, "#include \"%s\"\n", hFile.c_str());
    fprintf(c, "#include \"%s\"\n", cFile.c_str());
}

void Table::cAutogenComment(FILE *o) const
{
    fprintf(o, "/*\n");
    fprintf(o, " * THIS FILE WAS AUTOMATICALLY GENERATED; DO NOT EDIT!\n");
    fprintf(o, " *\n");
    fprintf(o, " *\n");
    fprintf(o, " * Generated by dbc, version 0.1\n"); // XXX version
    fprintf(o, " *\n");
    fprintf(o, " */\n");
}


void Table::genCtor(FILE *h, FILE *c) const
{
    // loop through columns, see if anything needs a zero init (integers, etc)
    std::vector<FString> initV;
    FString initStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        if (tc.cTypeNeedsZeroInit())
            initV.push_back(FString(FStringFC(), "%s(0)",tc.cName().c_str()));
    }
    initStr.Implode(", ",initV);

    fprintf(h, "    %s();\n", mClassname.c_str());
    if (initStr.empty())
        fprintf(c, "%s::%s()\n", mClassname.c_str(), mClassname.c_str());
    else
        fprintf(c, "%s::%s() :\n        %s\n", 
                mClassname.c_str(), mClassname.c_str(), initStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    dbc_original_loaded = false;\n");
    fprintf(c, "}\n");
}
void Table::genRowCtor(FILE *h, FILE *c) const
{
    fprintf(h, "    %s(const Forte::DbResultRow &row);\n", mClassname.c_str());
    fprintf(c, "%s::%s(const Forte::DbResultRow &row)\n", mClassname.c_str(), mClassname.c_str());
    fprintf(c, "{\n    Set(row);\n}\n");   
}
void Table::genPrimaryCtor(FILE *h, FILE *c) const
{
    // generate string and parameters to sql.Format() call
    std::vector<FString> formatStrV, formatParamV, paramV;
    FString formatStr, formatParams, paramStr;
    bool empty = true;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            empty = false;
            formatStrV.push_back(tc.cFormatStr());
            formatParamV.push_back(tc.cFormatParam(tc.mName));
            paramV.push_back(tc.mName);
        }
    }
    if (empty) return;
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);
    paramStr.Implode(", ", paramV);
    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
//        printf("  Column %s has options 0x%x\n", tc.mName.c_str(), tc.mOptions);
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            keyParamV.push_back(FString(FStringFC(), 
                                        "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
        }
    }
    keyParamStr.Implode(", ", keyParamV);

    fprintf(h, "    %s(Forte::DbConnection &db, %s);\n", mClassname.c_str(), keyParamStr.c_str());
    fprintf(c, "%s::%s(Forte::DbConnection &db, %s)\n", 
            mClassname.c_str(), mClassname.c_str(), keyParamStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::DbResult res = %s::SelectPrimaryKey(db, %s);\n",
            mClassname.c_str(), paramStr.c_str());
    fprintf(c, "    Forte::DbResultRow row;\n");
    fprintf(c, "    if (!res.FetchRow(row))\n");
    fprintf(c, "        throw Forte::DbException(\"invalid %s\");\n", paramStr.c_str());
    fprintf(c, "    Set(row);\n");
    fprintf(c, "}\n");
}
void Table::genDtor(FILE *h, FILE *c) const
{
    fprintf(h, "    ~%s() {};\n",mClassname.c_str());
}
void Table::genSet(FILE *h, FILE *c) const
{
    fprintf(h, "    virtual void Set(const Forte::DbResultRow &row);\n");
    fprintf(c, "void %s::Set(const Forte::DbResultRow &row)\n", mClassname.c_str());
    fprintf(c, "{\n");
    int i = 0;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
            fprintf(c, "    %s = %s = %s(row, %d);\n", tc.cName().c_str(), 
                    tc.cOriginalName().c_str(), tc.cGetter().c_str(), i++);
        else
            fprintf(c, "    %s = %s(row, %d);\n", tc.cName().c_str(), tc.cGetter().c_str(), i++);
    }
    fprintf(c, "    dbc_original_loaded = true;\n"); // mark as original loaded
    fprintf(c, "}\n");
}
void Table::genSetOrig(FILE *h, FILE *c) const
{
    fprintf(h, "    void SetOrig(void);\n");
    fprintf(c, "void %s::SetOrig(void)\n", mClassname.c_str());
    fprintf(c, "{\n");
//    int i = 0;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
            fprintf(c, "    %s = %s;\n", tc.cOriginalName().c_str(), tc.cName().c_str());
    }
    fprintf(c, "    dbc_original_loaded = true;\n"); // mark as original loaded
    fprintf(c, "}\n");
}
void Table::genSelect(FILE *h, FILE *c) const
{
    std::vector<FString> cols;
    FString colstr;
    foreach(YYSTYPE a, mColumns)
        cols.push_back(aliasPrefix() + TC(TableColumn,a).sqlName());
    colstr.Implode(", ", cols);

    fprintf(h, "    static Forte::DbResult Select(Forte::DbConnection &db, const char *appendSql = NULL);\n");
    fprintf(c, "Forte::DbResult %s::Select(Forte::DbConnection &db, const char *appendSql)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"SELECT %s FROM %s %s \");\n", 
            colstr.c_str(), mName.c_str(), (mAlias.empty())?"":mAlias.c_str());
    fprintf(c, "    if (appendSql) sql.append(appendSql);\n");
    fprintf(c, "    return Forte::DbUtil::DbStore(db, sql);\n");
    fprintf(c, "}\n");
}
void Table::genCount(FILE *h, FILE *c) const
{
    fprintf(h, "    static unsigned int Count(Forte::DbConnection &db, const char *appendSql = NULL);\n");
    fprintf(c, "unsigned int %s::Count(Forte::DbConnection &db, const char *appendSql)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"SELECT COUNT(*) FROM %s %s \");\n", 
            mName.c_str(), (mAlias.empty())?"":mAlias.c_str());
    fprintf(c, "    if (appendSql) sql.append(appendSql);\n");
    fprintf(c, "    Forte::DbResult res = Forte::DbUtil::DbStore(db, sql);\n");
    fprintf(c, "    Forte::DbResultRow row;\n");
    fprintf(c, "    if (res.FetchRow(row))\n");
    fprintf(c, "        return GetUInt(row, 0);\n");
    fprintf(c, "    else\n");
    fprintf(c, "        return 0;\n");
    fprintf(c, "}\n");
}
void Table::genSelectAlias(FILE *h, FILE *c) const
{
    std::vector<FString> cols;
    FString colstr;
    foreach(YYSTYPE a, mColumns)
        cols.push_back(aliasPrefix() + TC(TableColumn,a).sqlName());
    colstr.Implode(", ", cols);

    fprintf(h, "    static Forte::DbResult SelectAlias(Forte::DbConnection &db, const char *tableAlias = NULL, const char *appendSql = NULL);\n");
    fprintf(c, "Forte::DbResult %s::SelectAlias(Forte::DbConnection &db, const char *tableAlias, const char *appendSql)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString colstr;\n");
    fprintf(c, "    if (!tableAlias)\n");
    fprintf(c, "        colstr = \"%s\";\n", colstr.c_str());
    fprintf(c, "    else\n");
    fprintf(c, "    {\n");
    bool first = true;
    foreach(YYSTYPE a, mColumns)
    {
        fprintf(c, "        colstr.append(Forte::FString(Forte::FStringFC(),\"%s%%s.%s\",tableAlias));\n",
                first?"":", ", TC(TableColumn,a).sqlName().c_str());
        first = false;
    }
    fprintf(c, "    }\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"SELECT %%s FROM %s %%s \", colstr.c_str(), "
            "tableAlias?tableAlias:\"\");\n",
            mName.c_str());
    fprintf(c, "    if (appendSql) sql.append(appendSql);\n");
    fprintf(c, "    return Forte::DbUtil::DbStore(db, sql);\n");
    fprintf(c, "}\n");
}
void Table::genSelectPrimaryKey(FILE *h, FILE *c) const
{
    // generate string and parameters to sql.Format() call
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    bool empty = true;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            empty = false;
            formatStrV.push_back(tc.cFormatStr(aliasPrefix()));
            formatParamV.push_back(tc.cFormatParam(tc.mName));
        }
    }
    if (empty) return;
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    // generate parameters to function call
    std::vector<FString> keyParamV;
    FString keyParamStr;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            keyParamV.push_back(FString(FStringFC(), 
                                        "%s %s", tc.cPODType().c_str(), tc.mName.c_str()));
        }
    }
    keyParamStr.Implode(", ", keyParamV);

    fprintf(h, "    static Forte::DbResult SelectPrimaryKey(Forte::DbConnection &db, %s);\n", keyParamStr.c_str());
    fprintf(c, "Forte::DbResult %s::SelectPrimaryKey(Forte::DbConnection &db, %s)\n", 
            mClassname.c_str(), keyParamStr.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" WHERE %s \",\n",formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    return Select(db, sql);\n");
    fprintf(c, "}\n");
}

void Table::genCounts(FILE *h, FILE *c) const
{
    foreach (YYSTYPE a, mCounts)
    {
        TableCount &tc(TC(TableCount,a));
        tc.linkToColumns(*this);
        tc.generateCPP(*this, h, c);
    }
}
void Table::genSelectors(FILE *h, FILE *c) const
{
    foreach (YYSTYPE a, mSelectors)
    {
        TableSelector &ts(TC(TableSelector,a));
        ts.linkToColumns(*this);
        ts.generateCPP(*this, h, c);
    }
}
void Table::genDeletors(FILE *h, FILE *c) const
{
    foreach (YYSTYPE a, mDeletors)
    {
        TableDeletor &td(TC(TableDeletor,a));
        td.linkToColumns(*this);
        td.generateCPP(*this, h, c);
    }
}
void Table::genExistors(FILE *h, FILE *c) const
{
    foreach (YYSTYPE a, mExistors)
    {
        TableExistor &te(TC(TableExistor,a));
        te.linkToColumns(*this);
        te.generateCPP(*this, h, c);
    }
}
void Table::genLookups(FILE *h, FILE *c) const
{
    foreach (YYSTYPE a, mLookups)
    {
        TableLookup &tl(TC(TableLookup,a));
        tl.linkToColumns(*this);
        tl.generateCPP(*this, h, c);
    }
}
void Table::genCtors(FILE *h, FILE *c) const
{
    foreach (YYSTYPE a, mCtors)
    {
        TableCtor &tc(TC(TableCtor,a));
        tc.linkToColumns(*this);
        tc.generateCPP(*this, h, c);
    }
}

void Table::genReplace(FILE *h, FILE *c, bool key, bool set) const
{
    fprintf(h, "    void Replace(Forte::DbConnection &db);\n");
    fprintf(c, "void %s::Replace(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    if (mCreatedColumn && mModifiedColumn)
        fprintf(c, "    %s = %s = time(0);\n",
                TC(TableColumn,mCreatedColumn).cName().c_str(),
                TC(TableColumn,mModifiedColumn).cName().c_str());
    else if (mCreatedColumn)
        fprintf(c, "    %s = time(0);\n",
                TC(TableColumn,mCreatedColumn).cName().c_str());
    else if (mModifiedColumn)
        fprintf(c, "    %s = time(0);\n",
                TC(TableColumn,mModifiedColumn).cName().c_str());
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"REPLACE INTO %s \"\n", mName.c_str());

    if (key && set)
    {
        fprintf(c, "               \"(%%s, %%s) VALUES (%%s, %%s) \",\n");
        fprintf(c, "               keyColumnSql(db).c_str(), setColumnSql(db).c_str(),\n");
        fprintf(c, "               keyValueSql(db).c_str(), setValueSql(db).c_str());\n");
    }
    else if (key)
    {
        fprintf(c, "               \"(%%s) VALUES (%%s) \",\n");
        fprintf(c, "               keyColumnSql(db).c_str(), keyValueSql(db).c_str());\n");
    }
    else if (set)
    {
        fprintf(c, "               \"(%%s) VALUES (%%s)\",\n");
        fprintf(c, "               setColumnSql(db).c_str(), setValueSql(db).c_str());\n");
    }
    else // no setSql() call and no settable key columns.
        throw Exception(FStringFC(), "%s::insert() cannot be generated (has no setSql())",
                         mName.c_str());


//     if (key && set)
//     {
//         fprintf(c, "               \"SET %%s, %%s \",\n");
//         fprintf(c, "               keySql(db).c_str(), setSql(db).c_str());\n");
//     }
//     else if (key)
//     {
//         fprintf(c, "               \"SET %%s\",\n");
//         fprintf(c, "               keySql(db).c_str());\n");
//     }
//     else if (set)
//     {
//         fprintf(c, "               \"SET %%s\",\n");
//         fprintf(c, "               setSql(db).c_str());\n");
//     }
    fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");
    fprintf(c, "    SetOrig();\n");
    fprintf(c, "}\n");
    fprintf(c, "\n");
}
void Table::genInsert(FILE *h, FILE *c, bool keynoauto, bool set) const
{
    fprintf(h, "    void Insert(Forte::DbConnection &db);\n");
    fprintf(c, "void %s::Insert(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    if (mCreatedColumn && mModifiedColumn)
        fprintf(c, "    %s = %s = time(0);\n",
                TC(TableColumn,mCreatedColumn).cName().c_str(),
                TC(TableColumn,mModifiedColumn).cName().c_str());
    else if (mCreatedColumn)
        fprintf(c, "    %s = time(0);\n",
                TC(TableColumn,mCreatedColumn).cName().c_str());
    else if (mModifiedColumn)
        fprintf(c, "    %s = time(0);\n",
                TC(TableColumn,mModifiedColumn).cName().c_str());
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"INSERT INTO %s \"\n", mName.c_str());
    if (keynoauto && set)
    {
        fprintf(c, "               \"(%%s, %%s) VALUES (%%s, %%s) \",\n");
        fprintf(c, "               keyNoAutoColumnSql(db).c_str(), setColumnSql(db).c_str(),\n");
        fprintf(c, "               keyNoAutoValueSql(db).c_str(), setValueSql(db).c_str());\n");
    }
    else if (keynoauto)
    {
        fprintf(c, "               \"(%%s) VALUES (%%s) \",\n");
        fprintf(c, "               keyNoAutoColumnSql(db).c_str(), keyNoAutoValueSql(db).c_str());\n");
    }
    else if (set)
    {
        fprintf(c, "               \"(%%s) VALUES (%%s)\",\n");
        fprintf(c, "               setColumnSql(db).c_str(), setValueSql(db).c_str());\n");
    }
    else // no setSql() call and no settable key columns.
        throw Exception(FStringFC(), "%s::insert() cannot be generated (has no setSql())",
                         mName.c_str());

/////////////////////////////
//     if (keynoauto && set)
//     {
//         fprintf(c, "               \"SET %%s, %%s \",\n");
//         fprintf(c, "               keyNoAutoSql(db).c_str(), setSql(db).c_str());\n");
//     }
//     else if (keynoauto)
//     {
//         fprintf(c, "               \"SET %%s\",\n");
//         fprintf(c, "               keyNoAutoSql(db).c_str());\n");
//     }
//     else if (set)
//     {
//         fprintf(c, "               \"SET %%s\",\n");
//         fprintf(c, "               setSql(db).c_str());\n");
//     }
//     else // no setSql() call and no settable key columns.
//         throw Exception(FStringFC(), "%s::insert() cannot be generated (has no setSql())",
//                          mName.c_str());
//////////////////////////////

    fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");
    if (mAutoIncrementColumn)
        fprintf(c, "    %s = db.InsertID();\n", 
                TC(TableColumn,mAutoIncrementColumn).cName().c_str());
    // set original key data columns
    fprintf(c, "    SetOrig();\n");
    fprintf(c, "}\n");
    fprintf(c, "\n");
}
void Table::genUpdate(FILE *h, FILE *c, bool set, bool where) const
{
    if (!set || !where) return;
    fprintf(h, "    void Update(Forte::DbConnection &db);\n");
    fprintf(c, "void %s::Update(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    if (mModifiedColumn)
        fprintf(c, "    %s = time(0);\n",
                TC(TableColumn,mModifiedColumn).cName().c_str());
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"UPDATE %s \"\n", mName.c_str());
    fprintf(c, "               \"SET %%s \"\n");
    fprintf(c, "               \"WHERE %%s \",\n");
    fprintf(c, "               setSql(db).c_str(), whereSql(db).c_str());\n");
    fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");
    fprintf(c, "    SetOrig();\n");
    fprintf(c, "}\n");
    fprintf(c, "\n");
}
void Table::genFKInclude(FILE *c) const
{
    foreach (YYSTYPE k, mForeignKeys)
    {
        const ForeignKey &fk(TC(ForeignKey,k));
        if (fk.mFlags & FK_OPT_PRIMARY)
        {
            // this is the primary table in the foreign key
            // all other tables in the key must be included
            std::vector<YYSTYPE> keys;
            PC().getForeignKeysNamed(fk.mName, keys);
            foreach(YYSTYPE k2, keys)
            {
                const ForeignKey &other(TC(ForeignKey, k2));
                const Table &otherTable(TC(Table,other.mTable));
                fprintf(c, "#include \"%s%s.h\"\n",
                        PC().getFilenamePrefix().c_str(), otherTable.mFilenameBase.c_str());
            }
        }
    }
}
void Table::genUpdateKey(FILE *h, FILE *c, bool key, bool set, bool where) const
{
    if (!set || !where) return;
    fprintf(h, "    void UpdateKey(Forte::DbConnection &db);\n");
    fprintf(c, "void %s::UpdateKey(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");

    callFKRestrict(c, true);

    // update all columns (including primary key), use original values for primary key
    // also, foreign keys should be updated (if applicable)
    // update originals when complete
    if (mModifiedColumn)
        fprintf(c, "    %s = time(0);\n",
                TC(TableColumn,mModifiedColumn).cName().c_str());
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"UPDATE %s \"\n", mName.c_str());
    fprintf(c, "               \"SET %%s, %%s \"\n");
    fprintf(c, "               \"WHERE %%s \",\n");
    fprintf(c, "               keySql(db).c_str(), setSql(db).c_str(), origWhereSql(db).c_str());\n");
    fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");

    // update foreign keys
    foreach (YYSTYPE k, mForeignKeys)
    {
        const ForeignKey &fk(TC(ForeignKey,k));
        if (fk.mFlags & FK_OPT_PRIMARY)
        {
            // this is the primary table in the foreign key
            // all other tables must be updated
            std::vector<YYSTYPE> keys;
            PC().getForeignKeysNamed(fk.mName, keys);
            foreach(YYSTYPE k2, keys)
            {
                const ForeignKey &other(TC(ForeignKey, k2));
                const Table &otherTable(TC(Table,other.mTable));
                if (other.mFlags & FK_OPT_UPDATE_DELETE)
                {
                    fprintf(c, "    %s::deleteFK%s%d(db, %s);\n",
                            otherTable.mClassname.c_str(), fk.mName.c_str(), other.mID, fk.paramStr(other).c_str());
                }
                else if (other.mFlags & FK_OPT_UPDATE_CASCADE)
                    fprintf(c, "    %s::updateFK%s%d(db, %s, %s);\n",
                            otherTable.mClassname.c_str(), fk.mName.c_str(), other.mID,
                            fk.paramStr(other, false).c_str(),
                            fk.paramStr(other).c_str());
            }
        }
    }
    fprintf(c, "    SetOrig();\n");

    fprintf(c, "}\n");
    fprintf(c, "\n");
}
void Table::genRefresh(FILE *h, FILE *c, bool where) const
{
    if (!where) return;
    // generate string and parameters to sql.Format() call
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    bool empty = true;
    foreach(YYSTYPE a, mColumns)
    {
        const TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            empty = false;
            formatStrV.push_back(tc.cFormatStr(aliasPrefix()));
            formatParamV.push_back(tc.cFormatParam(tc.cName()));
        }
    }
    if (empty) return;
    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    fprintf(h, "    void Refresh(Forte::DbConnection &db);\n");
    fprintf(c, "void %s::Refresh(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" WHERE %s \",\n",formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    Forte::DbResult res = Select(db, sql);\n");
    fprintf(c, "    Forte::DbResultRow row;\n");
    fprintf(c, "    if (!res.FetchRow(row))\n");
    fprintf(c, "        throw Forte::DbException(\"unable to refresh object\");\n");
    fprintf(c, "    Set(row);\n");
    fprintf(c, "}\n");
    fprintf(c, "\n");
}

void Table::callFKRestrict(FILE *c, bool update) const
{
    // check foreign key restrictions
    // if update is true, we are updating, else deleting
    foreach (YYSTYPE k, mForeignKeys)
    {
        const ForeignKey &fk(TC(ForeignKey,k));
        if (fk.mFlags & FK_OPT_PRIMARY)
        {
            // this is the primary table in the foreign key
            std::vector<YYSTYPE> keys;
            PC().getForeignKeysNamed(fk.mName, keys);
            foreach(YYSTYPE k2, keys)
            {
                const ForeignKey &other(TC(ForeignKey, k2));
                const Table &otherTable(TC(Table,other.mTable));
                if ((update && (other.mFlags & FK_OPT_UPDATE_RESTRICT)) ||
                    (!update && (other.mFlags & FK_OPT_DELETE_RESTRICT)))
                {
                    fprintf(c, "    %s::restrictFK%s%d(db, %s);\n",
                            otherTable.mClassname.c_str(), 
                            fk.mName.c_str(), other.mID, fk.paramStr(other).c_str());
                }
            }
        }
    }
}
void Table::genFKRestrict(FILE *h, FILE *c) const
{
    // for each non primary foreign key
    foreach(YYSTYPE k, mForeignKeys)
    {
        const ForeignKey &fk(TC(ForeignKey,k));
        if (fk.mFlags & FK_OPT_PRIMARY) continue;
        // generate the where clause for this foreign key
        std::vector<FString> formatStrV, formatParamV;
        FString formatStr, formatParams;
        foreach(YYSTYPE cv,fk.mColValues)
        {
            const ColumnValue &colValue(TC(ColumnValue,cv));
            const TableColumn &tc(TC(TableColumn,colValue.mColumn));
            if (colValue.mValueSet)
            {
                formatStrV.push_back(FString(FStringFC(), "%s = %s", 
                                             tc.mName.c_str(), colValue.sqlValue().c_str()));
            }
            else
            {
                formatStrV.push_back(tc.cFormatStr());
                formatParamV.push_back(tc.cFormatParam(tc.mName));
            }
        }
        formatStr.Implode(" AND ", formatStrV);
        formatParams.Implode(", ", formatParamV);
        fprintf(h, "    static void restrictFK%s%d(Forte::DbConnection &db, %s);\n", 
                fk.mName.c_str(), fk.mID, fk.localParamStr().c_str());
        fprintf(c, "void %s::restrictFK%s%d(Forte::DbConnection &db, %s)\n", mClassname.c_str(),
                fk.mName.c_str(), fk.mID, fk.localParamStr().c_str());
        fprintf(c, "{\n");
        fprintf(c, "    Forte::FString sql;\n");
        fprintf(c, "    sql.Format(\"SELECT 1 FROM %s WHERE %s LIMIT 1 \",\n", 
                mName.c_str(), formatStr.c_str());
        fprintf(c, "               %s);\n", formatParams.c_str());
        fprintf(c, "    Forte::DbResult res = Forte::DbUtil::DbStore(db, sql);\n");
        fprintf(c, "    if (res.GetNumRows() != 0)\n");
        fprintf(c, "        throw Forte::DbException(\"Foreign key restriction\");\n");
        fprintf(c, "}\n");
    }
}
void Table::genFKUpdate(FILE *h, FILE *c) const
{
    std::set<FString> fKeys;

    // for each non primary foreign key
    foreach(YYSTYPE k, mForeignKeys)
    {
        const ForeignKey &fk(TC(ForeignKey,k));
        if (fk.mFlags & FK_OPT_PRIMARY) continue;
        // save the name of the key for the unified updators
        fKeys.insert(fk.mName);
        // generate the where clause for this foreign key
        std::vector<FString> formatStrV, formatParamV, setStrV, setParamV;
        FString formatStr, formatParams, setStr, setParams;
        foreach(YYSTYPE cv,fk.mColValues)
        {
            const ColumnValue &colValue(TC(ColumnValue,cv));
            const TableColumn &tc(TC(TableColumn,colValue.mColumn));
            if (colValue.mValueSet)
            {
                formatStrV.push_back(FString(FStringFC(), "%s = %s", 
                                             tc.mName.c_str(), colValue.sqlValue().c_str()));
            }
            else
            {
                formatStrV.push_back(tc.cFormatStr());
                formatParamV.push_back(tc.cFormatParam("orig_" + tc.mName));
                setStrV.push_back(tc.cFormatStr());
                setParamV.push_back(tc.cFormatParam(tc.mName));
            }
        }
        formatStr.Implode(" AND ", formatStrV);
        formatParams.Implode(", ", formatParamV);
        setStr.Implode(", ", setStrV);
        setParams.Implode(", ", setParamV);
        fprintf(h, "    static void updateFK%s%d(Forte::DbConnection &db, %s, %s);\n", 
                fk.mName.c_str(), fk.mID, fk.localParamStr().c_str(), fk.localParamStr("orig_").c_str());
        fprintf(c, "void %s::updateFK%s%d(Forte::DbConnection &db, %s, %s)\n", mClassname.c_str(),
                fk.mName.c_str(), fk.mID, fk.localParamStr().c_str(),fk.localParamStr("orig_").c_str());
        fprintf(c, "{\n");
        fprintf(c, "    Forte::FString sql;\n");
        fprintf(c, "    sql.Format(\"UPDATE %s SET %s WHERE %s \",\n", 
                mName.c_str(), setStr.c_str(), formatStr.c_str());
        fprintf(c, "               %s, %s);\n", setParams.c_str(), formatParams.c_str());
        fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");
        fprintf(c, "}\n");
    }

    // Generator the unified updators
    // Yes, these loops are not efficient, but I dont feel like setting up the proper data structures.
    foreach (const FString &keyName, fKeys)
    {
        // find any matching foreign key so we can get the params
        foreach(YYSTYPE k, mForeignKeys)
        {
            const ForeignKey &fk(TC(ForeignKey,k));
            if (fk.mFlags & FK_OPT_PRIMARY) continue;
            if (fk.mName.Compare(keyName)) continue;
            fprintf(h, "    static void updateFK%s(Forte::DbConnection &db, %s, %s);\n",
                    keyName.c_str(),  fk.localParamStr().c_str(),fk.localParamStr("orig_").c_str());
            fprintf(c, "void %s::updateFK%s(Forte::DbConnection &db, %s, %s)\n",
                    mClassname.c_str(),
                    keyName.c_str(),  fk.localParamStr().c_str(),fk.localParamStr("orig_").c_str());
            fprintf(c, "{\n");
            // for each non primary foreign key
            foreach(YYSTYPE k2, mForeignKeys)
            {
                const ForeignKey &fk2(TC(ForeignKey,k2));
                if (fk2.mFlags & FK_OPT_PRIMARY) continue;
                if (fk2.mName.Compare(keyName)) continue;

                std::vector<FString> formatParamV, setParamV;
                FString formatParams, setParams;
                foreach(YYSTYPE cv,fk.mColValues)
                {
                    const ColumnValue &colValue(TC(ColumnValue,cv));
                    const TableColumn &tc(TC(TableColumn,colValue.mColumn));
                    if (!colValue.mValueSet)
                    {
                        formatParamV.push_back(tc.cFormatParam("orig_" + tc.mName));
                        setParamV.push_back(tc.cFormatParam(tc.mName));
                    }
                }
                formatParams.Implode(", ", formatParamV);
                setParams.Implode(", ", setParamV);

                fprintf(c, "    updateFK%s%d(db, %s, %s);\n", keyName.c_str(), fk2.mID,
                        setParams.c_str(), formatParams.c_str());
            }
            fprintf(c, "}\n");
            break; // only need one!
        }
    }
}
void Table::genFKDelete(FILE *h, FILE *c) const
{
    std::set<FString> fKeys;

    // for each non primary foreign key
    foreach(YYSTYPE k, mForeignKeys)
    {
        const ForeignKey &fk(TC(ForeignKey,k));
        if (fk.mFlags & FK_OPT_PRIMARY) continue;
        // save the name of the key for the unified deletors
        fKeys.insert(fk.mName);
        // generate the where clause for this foreign key
        std::vector<FString> formatStrV, formatParamV;
        FString formatStr, formatParams;
        foreach(YYSTYPE cv,fk.mColValues)
        {
            const ColumnValue &colValue(TC(ColumnValue,cv));
            const TableColumn &tc(TC(TableColumn,colValue.mColumn));
            if (colValue.mValueSet)
            {
                formatStrV.push_back(FString(FStringFC(), "%s = %s", 
                                             tc.mName.c_str(), colValue.sqlValue().c_str()));
            }
            else
            {
                formatStrV.push_back(tc.cFormatStr());
                formatParamV.push_back(tc.cFormatParam(tc.mName));
            }
        }
        formatStr.Implode(" AND ", formatStrV);
        formatParams.Implode(", ", formatParamV);
        fprintf(h, "    static void deleteFK%s%d(Forte::DbConnection &db, %s);\n", 
                fk.mName.c_str(), fk.mID, fk.localParamStr().c_str());
        fprintf(c, "void %s::deleteFK%s%d(Forte::DbConnection &db, %s)\n", mClassname.c_str(),
                fk.mName.c_str(), fk.mID, fk.localParamStr().c_str());
        fprintf(c, "{\n");
        fprintf(c, "    Forte::FString sql;\n");
        fprintf(c, "    sql.Format(\"DELETE FROM %s WHERE %s \",\n", 
                mName.c_str(), formatStr.c_str());
        fprintf(c, "               %s);\n", formatParams.c_str());
        fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");
        fprintf(c, "}\n");
    }


    // Generator the unified deletors
    // Yes, these loops are not efficient, but I dont feel like setting up the proper data structures.
    foreach (const FString &keyName, fKeys)
    {
        // find any matching foreign key so we can get the params
        foreach(YYSTYPE k, mForeignKeys)
        {
            const ForeignKey &fk(TC(ForeignKey,k));
            if (fk.mFlags & FK_OPT_PRIMARY) continue;
            if (fk.mName.Compare(keyName)) continue;
            fprintf(h, "    static void deleteFK%s(Forte::DbConnection &db, %s);\n",
                    keyName.c_str(),  fk.localParamStr().c_str());
            fprintf(c, "void %s::deleteFK%s(Forte::DbConnection &db, %s)\n",
                    mClassname.c_str(),
                    keyName.c_str(),  fk.localParamStr().c_str());
            fprintf(c, "{\n");
            // for each non primary foreign key
            foreach(YYSTYPE k2, mForeignKeys)
            {
                const ForeignKey &fk2(TC(ForeignKey,k2));
                if (fk2.mFlags & FK_OPT_PRIMARY) continue;
                if (fk2.mName.Compare(keyName)) continue;

                std::vector<FString> formatParamV;
                FString formatParams;
                foreach(YYSTYPE cv,fk.mColValues)
                {
                    const ColumnValue &colValue(TC(ColumnValue,cv));
                    const TableColumn &tc(TC(TableColumn,colValue.mColumn));
                    if (!colValue.mValueSet)
                    {
                        formatParamV.push_back(tc.cFormatParam(tc.mName));
                    }
                }
                formatParams.Implode(", ", formatParamV);

                fprintf(c, "    deleteFK%s%d(db, %s);\n", keyName.c_str(), fk2.mID,
                        formatParams.c_str());
            }
            fprintf(c, "}\n");
            break; // only need one!
        }
    }

}
void Table::genDeleteMe(FILE *h, FILE *c, bool where) const
{
    // XXX delete me should compare the original values to the current
    //     if there is any difference, the operation should be rejected.

    // delete foreign keys if applicable
    if (!where) return;
    fprintf(h, "    void DeleteMe(Forte::DbConnection &db, bool cascade = true);\n");
    fprintf(c, "void %s::DeleteMe(Forte::DbConnection &db, bool cascade)\n", mClassname.c_str());
    fprintf(c, "{\n");
    
    fprintf(c, "    if (cascade) {\n");
    callFKRestrict(c, false);
    fprintf(c, "    }\n");
    
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\"DELETE FROM %s \"\n", mName.c_str());
    fprintf(c, "               \"WHERE %%s \",\n");
    fprintf(c, "               whereSql(db).c_str());\n");
    fprintf(c, "    Forte::DbUtil::DbExecute(db, sql);\n");

    // do foreign key deletes
    fprintf(c, "    if (cascade)\n    {\n");
    foreach (YYSTYPE k, mForeignKeys)
    {
        const ForeignKey &fk(TC(ForeignKey,k));
        if (fk.mFlags & FK_OPT_PRIMARY)
        {
            // this is the primary table in the foreign key
            // all other tables must be updated
            std::vector<YYSTYPE> keys;
            PC().getForeignKeysNamed(fk.mName, keys);
            foreach(YYSTYPE k2, keys)
            {
                const ForeignKey &other(TC(ForeignKey, k2));
                const Table &otherTable(TC(Table,other.mTable));
                if (other.mFlags & FK_OPT_DELETE_CASCADE)
                {
                    fprintf(c, "        %s::deleteFK%s%d(db, %s);\n",
                            otherTable.mClassname.c_str(), fk.mName.c_str(), 
                            other.mID, fk.paramStr(other).c_str());
                }
            }
        }
    }
    fprintf(c, "    }\n");
    fprintf(c, "}\n");
    fprintf(c, "\n");
}
bool Table::genSetSql(FILE *h, FILE *c) const
{
    if (mOptions & OPT_CUSTOMSET) return true;
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    bool empty = true;
    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if ((tc.mOptions & OPT_IN_PRIMARY_KEY) == 0)
        {
            empty = false;
            formatStrV.push_back(tc.cFormatStr());
            formatParamV.push_back(tc.cFormatParam(tc.cName()));
        }
    }
    if (empty || (mOptions & OPT_NOSETSQL)) return false;
    formatStr.Implode(", ", formatStrV);
    formatParams.Implode(", ", formatParamV);
    fprintf(h, "    Forte::FString setSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::setSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");
    return true;
}
bool Table::genKeySql(FILE *h, FILE *c) const
{
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    bool empty = true;

    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            empty = false;
            formatStrV.push_back(tc.cFormatStr());
            formatParamV.push_back(tc.cFormatParam(tc.cName()));
        }
    }

    formatStr.Implode(", ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    if (empty || (mOptions & OPT_NOKEYSQL)) return false;
    fprintf(h, "    Forte::FString keySql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::keySql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");
    return true;
}
bool Table::genKeyNoAutoSql(FILE *h, FILE *c) const
{
    // generate primary key sql (without the auto-increment field)
    std::vector<FString> formatStrV, formatParamV;
    FString formatStr, formatParams;
    bool empty = true;

    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY &&
            !(tc.mOptions & OPT_AUTO_INCREMENT))
        {
            empty = false;
            formatStrV.push_back(tc.cFormatStr());
            formatParamV.push_back(tc.cFormatParam(tc.cName()));
        }
    }

    if (empty || (mOptions & OPT_NOKEYSQL)) return false;

    formatStr.Implode(", ", formatStrV);
    formatParams.Implode(", ", formatParamV);

    fprintf(h, "    Forte::FString keyNoAutoSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::keyNoAutoSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");
    return true;
}

bool Table::genSetColumnSql(FILE *h, FILE *c) const
{
    // generate a list of all columns for use in a SQL query
    if (mOptions & OPT_CUSTOMSET) return true;
    std::vector<FString> columnStrV;
    FString columnStr;
    bool empty = true;
    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if ((tc.mOptions & OPT_IN_PRIMARY_KEY) == 0)
        {
            empty = false;
            columnStrV.push_back(tc.sqlName());
        }
    }
    columnStr.Implode(", ", columnStrV);
    if (empty || (mOptions & OPT_NOSETSQL)) return false;

    fprintf(h, "    Forte::FString setColumnSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::setColumnSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    return \"%s\";\n", columnStr.c_str());
    fprintf(c, "}\n\n");
    return true;
}
bool Table::genSetValueSql(FILE *h, FILE *c) const
{
    // generate a list of all column values for use in a SQL query
    if (mOptions & OPT_CUSTOMSET) return true;
    std::vector<FString> valueStrV;
    std::vector<FString> paramStrV;
    FString valueStr;
    FString paramStr;
    bool empty = true;
    
    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if ((tc.mOptions & OPT_IN_PRIMARY_KEY) == 0)
        {
            empty = false;
            valueStrV.push_back(tc.cFormatElement());
            paramStrV.push_back(tc.cFormatParam(tc.cName()));
        }
    }
    valueStr.Implode(", ", valueStrV);
    paramStr.Implode(", ", paramStrV);
    if (empty || (mOptions & OPT_NOSETSQL)) return false;

    fprintf(h, "    Forte::FString setValueSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::setValueSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", valueStr.c_str());
    fprintf(c, "               %s);\n", paramStr.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");
    return true;
}
bool Table::genKeyColumnSql(FILE *h, FILE *c) const
{
    // generate a list of all PRIMARY KEY columns for use in a SQL query
    std::vector<FString> columnStrV;
    FString columnStr;
    bool empty = true;
    
    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            empty = false;
            columnStrV.push_back(tc.sqlName());
        }
    }
    columnStr.Implode(", ", columnStrV);

    if (empty || (mOptions & OPT_NOKEYSQL)) return false;
    fprintf(h, "    Forte::FString keyColumnSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::keyColumnSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    return \"%s\";\n", columnStr.c_str());
    fprintf(c, "}\n\n");
    return true;
    
}
bool Table::genKeyValueSql(FILE *h, FILE *c) const
{
    // generate a list of all PRIMARY KEY column values for use in a SQL query
    std::vector<FString> valueStrV;
    std::vector<FString> paramStrV;
    FString valueStr;
    FString paramStr;
    bool empty = true;

    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            empty = false;
            valueStrV.push_back(tc.cFormatElement());
            paramStrV.push_back(tc.cFormatParam(tc.cName()));
        }
    }
    valueStr.Implode(", ", valueStrV);
    paramStr.Implode(", ", paramStrV);

    if (empty || (mOptions & OPT_NOKEYSQL)) return false;
    fprintf(h, "    Forte::FString keyValueSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::keyValueSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", valueStr.c_str());
    fprintf(c, "               %s);\n", paramStr.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");
    return true;

}
bool Table::genKeyNoAutoColumnSql(FILE *h, FILE *c) const
{
    // generate a list of all non-auto-increment PRIMARY KEY columns
    // for use in a SQL query
    std::vector<FString> columnStrV;
    FString columnStr;
    bool empty = true;
    
    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY &&
            !(tc.mOptions & OPT_AUTO_INCREMENT))
        {
            empty = false;
            columnStrV.push_back(tc.sqlName());
        }
    }
    columnStr.Implode(", ", columnStrV);

    if (empty || (mOptions & OPT_NOKEYSQL)) return false;
    fprintf(h, "    Forte::FString keyNoAutoColumnSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::keyNoAutoColumnSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    return \"%s\";\n", columnStr.c_str());
    fprintf(c, "}\n\n");
    return true;

}
bool Table::genKeyNoAutoValueSql(FILE *h, FILE *c) const
{
    // generate a list of all non-auto-increment PRIMARY KEY column
    // values for use in a SQL query
    std::vector<FString> valueStrV;
    std::vector<FString> paramStrV;
    FString valueStr;
    FString paramStr;
    bool empty = true;

    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY &&
            !(tc.mOptions & OPT_AUTO_INCREMENT))
        {
            empty = false;
            valueStrV.push_back(tc.cFormatElement());
            paramStrV.push_back(tc.cFormatParam(tc.cName()));
        }
    }
    valueStr.Implode(", ", valueStrV);
    paramStr.Implode(", ", paramStrV);

    if (empty || (mOptions & OPT_NOKEYSQL)) return false;
    fprintf(h, "    Forte::FString keyNoAutoValueSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::keyNoAutoValueSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", valueStr.c_str());
    fprintf(c, "               %s);\n", paramStr.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");
    return true;

}


bool Table::genWhereSql(FILE *h, FILE *c) const
{
    std::vector<FString> formatStrV, formatParamV, formatParamOrigV;
    FString formatStr, formatParams, formatParamsOrig;
    bool empty = true;
    foreach(YYSTYPE a, mColumns)
    {
        TableColumn &tc(TC(TableColumn,a));
        if (tc.mOptions & OPT_IN_PRIMARY_KEY)
        {
            empty = false;
            formatStrV.push_back(tc.cFormatStr());
            formatParamV.push_back(tc.cFormatParam(tc.cName()));
            formatParamOrigV.push_back(tc.cFormatParam(tc.cOriginalName()));
        }
    }

    if (empty || (mOptions & OPT_NOWHERESQL)) return false;

    formatStr.Implode(" AND ", formatStrV);
    formatParams.Implode(", ", formatParamV);
    formatParamsOrig.Implode(", ", formatParamOrigV);

    fprintf(h, "    Forte::FString whereSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::whereSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", formatStr.c_str());
    fprintf(c, "               %s);\n", formatParams.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");

    fprintf(h, "    Forte::FString origWhereSql(Forte::DbConnection &db);\n");
    fprintf(c, "Forte::FString %s::origWhereSql(Forte::DbConnection &db)\n", mClassname.c_str());
    fprintf(c, "{\n");
    fprintf(c, "    Forte::FString sql;\n");
    fprintf(c, "    sql.Format(\" %s \",\n", formatStr.c_str());
    fprintf(c, "               %s);\n", formatParamsOrig.c_str());
    fprintf(c, "    return sql;\n");
    fprintf(c, "}\n\n");
    return true;
}

FString TableColumn::cDecl(void) const
{
    return cTypeName() + " " + cName() + ";";
}
FString TableColumn::cOriginalDecl(void) const
{
    return cTypeName() + " " + cOriginalName() + ";";
}

FString TableColumn::cTypeName(void) const
{
    FString var;
    // output type name
    if (mType->GetType() == typeid(BigIntType)) {
        if (mOptions & OPT_UNSIGNED) var += "unsigned ";
        var += "long long";
    } else if (mType->GetType() == typeid(BlobType) ||
               mType->GetType() == typeid(CharType) ||
               mType->GetType() == typeid(VarCharType) ||
               mType->GetType() == typeid(LongTextType) ||
               mType->GetType() == typeid(TextType)) {
        var += "Forte::FString";
    } else if (mType->GetType() == typeid(BooleanType)) {
        var += "bool";
    } else if (mType->GetType() == typeid(IntType) ||
               mType->GetType() == typeid(TinyIntType)) {
        if (mOptions & OPT_UNSIGNED) var += "unsigned ";
        var += "int";
    }
    return var;
}
bool TableColumn::cTypeNeedsZeroInit(void) const
{
     // output type name
    if (mType->GetType() == typeid(BigIntType) ||
        mType->GetType() == typeid(BooleanType) ||
        mType->GetType() == typeid(IntType) ||
        mType->GetType() == typeid(TinyIntType))
        return true;
    else 
        return false;
}
FString TableColumn::cPODType(void) const
{
    FString var;
    // output POD type name
    if (mType->GetType() == typeid(BigIntType)) {
        if (mOptions & OPT_UNSIGNED) var += "unsigned ";
        var += "long long";
    } else if (mType->GetType() == typeid(BlobType) ||
               mType->GetType() == typeid(CharType) ||
               mType->GetType() == typeid(VarCharType) ||
               mType->GetType() == typeid(LongTextType) ||
               mType->GetType() == typeid(TextType)) {
        var += "const char *";
    } else if (mType->GetType() == typeid(BooleanType)) {
        var += "bool";
    } else if (mType->GetType() == typeid(IntType) ||
               mType->GetType() == typeid(TinyIntType)) {
        if (mOptions & OPT_UNSIGNED) var += "unsigned ";
        var += "int";
    }
    return var;
}
FString TableColumn::cGetter(void) const
{
    // output getter method
    if (mType->GetType() == typeid(BigIntType)) {
        if (mOptions & OPT_UNSIGNED) return "GetULLInt";
        else return "GetLLInt";
    } else if (mType->GetType() == typeid(BlobType) ||
               mType->GetType() == typeid(CharType) ||
               mType->GetType() == typeid(VarCharType) ||
               mType->GetType() == typeid(LongTextType) ||
               mType->GetType() == typeid(TextType)) {
        return "GetString";
    } else if (mType->GetType() == typeid(BooleanType)) {
        return "GetBool";
    } else if (mType->GetType() == typeid(IntType) ||
               mType->GetType() == typeid(TinyIntType)) {
        if (mOptions & OPT_UNSIGNED) return "GetUInt";
        else return "GetInt";
    }
    throw Exception("Unknown getter");
}
FString TableColumn::cOriginalName(void) const
{
    return "dbc_original_" + cName();
}
FString TableColumn::cName(void) const
{
    // convert column name from something like category_id to mCategoryID
    FString name;
    name = "m";
    std::vector<FString> words;
    mName.Explode("_", words);
    foreach(FString word, words)
    {
        // define some words which are always full upper case
        if (!word.CompareNoCase("id") ||
            !word.CompareNoCase("ou") ||
            !word.CompareNoCase("guid") ||
            !word.CompareNoCase("xml") ||
            !word.CompareNoCase("xsl") ||
            !word.CompareNoCase("css")) 
            word.MakeUpper();
        else
            // capitalize the word
            word.Format("%s%s",word.Left(1).MakeUpper().c_str(), word.Mid(1).c_str());
        name.append(word);
    }
    return name;
}
FString TableColumn::cFormatStr(const char *aliasPrefix) const
{
    FString str;
    str.Format("%s%s = ", aliasPrefix, sqlName().c_str());
    str += cFormatElement();
    return str;
}
FString TableColumn::cFormatElement(void) const
{
    FString str;
    if (mType->GetType() == typeid(BigIntType)) {
        if (mOptions & OPT_UNSIGNED) str += "%llu";
        else str += "%ll";
    } else if (mType->GetType() == typeid(BlobType) ||
               mType->GetType() == typeid(CharType) ||
               mType->GetType() == typeid(VarCharType) ||
               mType->GetType() == typeid(LongTextType) ||
               mType->GetType() == typeid(TextType)) {
        str += "'%s'";
    } else if (mType->GetType() == typeid(BooleanType)) {
        str += "%u";
    } else if (mType->GetType() == typeid(IntType) ||
               mType->GetType() == typeid(TinyIntType)) {
        if (mOptions & OPT_UNSIGNED) str += "%u";
        else str += "%d";
    }
    else throw Exception("Unknown type in cFormatStr()");
    return str;
}
FString TableColumn::cFormatParam(const FString &name) const
{
    FString param;
    if (mType->GetType() == typeid(BlobType) ||
        mType->GetType() == typeid(CharType) ||
        mType->GetType() == typeid(VarCharType) ||
        mType->GetType() == typeid(LongTextType) ||
        mType->GetType() == typeid(TextType)) {
        param.Format("Forte::DbUtil::DbEscape(db, %s).c_str()", name.c_str());
    } else if (mType->GetType() == typeid(BooleanType) ||
               mType->GetType() == typeid(BigIntType) ||
               mType->GetType() == typeid(IntType) ||
               mType->GetType() == typeid(TinyIntType)) {
        param = name;
    }
    else throw Exception("Unknown type in cFormatStr()");
    return param;
}
FString TableColumn::sqlName(void) const
{
    if (!mName.compare("key"))
        return "`key`";
    else if (!mName.compare("order"))
        return "`order`";
    else if (!mName.compare("interval"))
        return "`interval`";
    else if (!mName.compare("index"))
        return "`index`";
    else
        return mName;
}
