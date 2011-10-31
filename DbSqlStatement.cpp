#include "DbSqlStatement.h"
#include "FString.h"

using namespace Forte;
using namespace std;

const FString& DbSqlStatement::GetStatement() const
{
    return mStatement;
}

bool DbSqlStatement::IsMutator() const
{
    return mIsMutator;
}

DbSqlStatement::~DbSqlStatement()
{
}

DbSqlStatement::DbSqlStatement(const FString& statement, const bool& isMutator)
    :mStatement(statement), mIsMutator(isMutator)
{
}

std::ostream& operator<<(ostream& out, const Forte::DbSqlStatement& obj)
{
    out << obj.GetStatement();
    return out;
}

