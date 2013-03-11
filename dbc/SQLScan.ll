%top {
#pragma GCC diagnostic ignored "-Wold-style-cast"
}

%{

#include "Forte.h"
#include "boost/any.hpp"
#include "boost/shared_ptr.hpp"
#include "InternalRep.h"
#include "AnyPtr.h"

int lineno = 1;
    void yyerror(const char *s);
        int yyparse(void);

using namespace DBC;

#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED
typedef boost::shared_ptr<AnyPtr> YYSTYPE;
#endif

#include "SQLParse.hpp"

#define LVAL(type) yylval = YYSTYPE(new AnyPtr(new type(yytext)))

%}

%option case-insensitive

%%

`[^`\n]*`     { LVAL(FString); TC(FString, yylval).Trim("`"); return IDENT_QUOTED; }
`[^`\n]*$     { yyerror("Unterminated backtick string"); }

@[A-Za-z][A-Za-z0-9_]*  { LVAL(FString); return IDENT_VAR; }

ALL           { return ALL; }
AUTO_INCREMENT { return AUTO_INCREMENT; }
AUTOINCREMENT { return AUTO_INCREMENT; }
BEGIN         { return BEGIN_TOK; }
BIGINT        { return BIGINT; }
BINARY        { return BINARY; }
BLOB          { return BLOB; }
BOOLEAN       { return BOOLEAN; }
BY            { return BY; }
CASCADE       { return CASCADE; }
CHAR          { return CHAR; }
CHARSET       { return CHARSET; }
CREATE        { return CREATE; }
COMMIT        { return COMMIT_TOK; }
COUNT         { return COUNT; }
DATABASE      { return DATABASE; }
DEFAULT       { return DEFAULT; }
DELETE        { return DELETE; }
DROP          { return DROP; }
ENGINE        { return ENGINE; }
EXCLUSIVE     { return EXCLUSIVE_TOK; }
EXISTS        { return EXISTS; }
GRANT         { return GRANT; }
IDENTIFIED    { return IDENTIFIED; }
IF            { return IF; }
INDEX         { return INDEX; }
INSERT        { return INSERT; }
INT           { return INT; }
INTEGER       { return INT; }
INTO          { return INTO; }
KEY           { return KEY; }
LONGTEXT      { return LONGTEXT; }
LOOKUP        { return LOOKUP; }
ON            { return ON; }
NOT           { return NOT; }
NULL          { return NULL_TOK; }
TABLE         { return TABLE; }
TEXT          { return TEXT; }
TINYINT       { return TINYINT; }
TO            { return TO; }
PRIMARY       { return PRIMARY; }
PRIVILEGES    { return PRIVILEGES; }
REPLACE       { return REPLACE; }
RESTRICT      { return RESTRICT; }
SET           { return SET; }
TRANSACTION   { return TRANSACTION_TOK; }
UNIQUE        { return UNIQUE; }
UNSIGNED      { return UNSIGNED; }
UPDATE        { return UPDATE; }
USE           { return USE; }
VALUES        { return VALUES; }
VARBINARY     { return VARBINARY; }
VARCHAR       { return VARCHAR; }

DEFINE        { return DEFINE; }
FILENAME      { return FILENAME; }
CLASSNAME     { return CLASSNAME; }
SELECTOR      { return SELECTOR; }
DELETOR       { return DELETOR; }
CTOR          { return CTOR; }
EXISTOR       { return EXISTOR; }
FK            { return FK; }
LOCK_UPDATE   { return LOCK_UPDATE; }
LOCK_SHARED   { return LOCK_SHARED; }
LOCK_OPTIONAL { return LOCK_OPTIONAL; }
NOUPDATE      { return NOUPDATE; }
NOINSERT      { return NOINSERT; }
NOREPLACE     { return NOREPLACE; }
NODELETE      { return NODELETE; }
NOSETSQL      { return NOSETSQL; }
NOKEYSQL      { return NOKEYSQL; }
NOWHERESQL    { return NOWHERESQL; }
NOCTOR        { return NOCTOR; }
CUSTOM        { return CUSTOM; }
CUSTOMSET     { return CUSTOMSET; }
ALIAS         { return ALIAS; }
INCLUDE       { return INCLUDE; }

"-- ="         { return PARAMS; }

"="           { return EQ; }
"<"           { return LT; }
"<="          { return LTE; }
">"           { return GT; }
">="          { return GTE; }

[-+*/:(),.;%@]  { return yytext[0]; }

[A-Za-z][A-Za-z0-9_]*  { LVAL(FString); return IDENT; }

[0-9]+ |
[0-9]+"."[0-9]* |
"."[0-9]*     { LVAL(IntNum); return INTNUM; }

"0x"[0-9a-fA-F]* { LVAL(HexNum); return HEXNUM; }

'[^'\n]*' |
\"[^\"\n]*\"     { LVAL(FString); TC(FString, yylval).Trim("'\""); return STRING; }

'[^'\n]*$     { yyerror("Unterminated String"); }
\"[^\"\n]*$   { yyerror("Unterminated String"); }

\n            { lineno++; }

[ \t\r]+    ;  /* whitespace */

"-- "([^=\n].*)?      ; { } /* comment */

.             yyerror("Invalid character");

%%

void yyerror(const char *s)
{
        printf("Line %d: %s at %s\n", lineno, s, yytext);
}

