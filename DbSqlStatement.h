#ifndef FORTE_DB_SQL_STATEMENT
#define FORTE_DB_SQL_STATEMENT

#include <iosfwd>
#include "Object.h"
#include "FString.h"

namespace Forte {

    class DbSqlStatement : public Object
    {
    public:
        virtual const FString& GetStatement() const;
        virtual bool IsMutator() const;
        virtual ~DbSqlStatement();

    protected:
        DbSqlStatement(const FString& statement, const bool& isMutator);

    private:
        const FString mStatement;
        const bool mIsMutator;
    }; // DbSqlStatement


    template <bool M>
    class DbSqlStatementTpl : public DbSqlStatement
    {
    public:
        typedef DbSqlStatement base_type;

        DbSqlStatementTpl(const FString& statement) : base_type(statement, M)
        {
        }
    }; // DbSqlStatementTpl


    typedef DbSqlStatementTpl<true> MutatorDbSqlStatement;
    typedef DbSqlStatementTpl<false> NonMutatorDbSqlStatement;

    typedef MutatorDbSqlStatement InsertDbSqlStatement;
    typedef MutatorDbSqlStatement UpdateDbSqlStatement;
    typedef MutatorDbSqlStatement CreateDbSqlStatement;
    typedef MutatorDbSqlStatement DeleteDbSqlStatement;
    typedef MutatorDbSqlStatement ReplaceDbSqlStatement;
    typedef NonMutatorDbSqlStatement SelectDbSqlStatement;

} // namespace Forte


std::ostream& operator<<(std::ostream& out, const Forte::DbSqlStatement& obj);

#endif // FORTE_DB_SQL_STATEMENT


