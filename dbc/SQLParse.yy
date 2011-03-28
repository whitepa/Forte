%{

#include "Forte.h"
#include "boost/any.hpp"
#include "boost/shared_ptr.hpp"
#include "InternalRep.h"
#include "AnyPtr.h"

int yylex(void);
void yyerror(char *s);

#define YYSTYPE_IS_DECLARED
typedef boost::shared_ptr<AnyPtr> YYSTYPE;

using namespace DBC;

%}

%token IDENT IDENT_QUOTED IDENT_VAR
%token STRING
%token HEXNUM
%token INTNUM

%left NOT
%left EQ GT GTE LT LTE
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%token ALL AUTO_INCREMENT BIGINT BINARY BLOB BOOLEAN BT_STRING BY 
%token CHAR CHARSET CREATE COUNT DATABASE DEFAULT DROP
%token ENGINE EXISTS GRANT IDENTIFIED IF INSERT INT INTO KEY 
%token LONGTEXT NULL_TOK ON TABLE TEXT TINYINT TO PRIMARY
%token PRIVILEGES REPLACE SET UNIQUE UNSIGNED USE VALUES VARBINARY VARCHAR

%token PARAMS DEFINE FILENAME CLASSNAME
%token SELECTOR DELETOR CTOR EXISTOR LOOKUP FK
%token LOCK_UPDATE LOCK_SHARED LOCK_OPTIONAL
%token NOUPDATE NOREPLACE NOINSERT NOSETSQL NOKEYSQL NOWHERESQL NOCTOR NODELETE CUSTOM CUSTOMSET
%token ALIAS INCLUDE
%token DELETE UPDATE CASCADE RESTRICT

%%

sql_list:
           sql ';' opt_params
         | sql_list sql ';' opt_params
         ;

sql: query

query:
           create
        |  drop
        |  grant
        |  insert
        |  replace
        |  set
        |  use
        ;

opt_params:
                                        { }
        |  opt_params PARAMS opt_param_list { }
        ;

opt_param_list:
        |  param_list
        ;

param_list:
           param                     { }
        |  param_list ',' param      { }
        ;

param:
           DEFINE '(' opt_ident_or_text ')'     { LASTTABLE().addDefine(TC(FString,$3));};
        |  FILENAME '(' STRING ')'   { LASTTABLE().setOutputFilename(TC(FString,$3));};
        |  CLASSNAME '(' STRING ')'  { LASTTABLE().setOutputClassname(TC(FString,$3)); };
        |  COUNT IDENT '(' column_list ')' { LASTTABLE().addCount($2, $4); };
        |  SELECTOR IDENT '(' column_list ')' selector_flags 
              { LASTTABLE().addSelector($2, $4, $6); };
        |  DELETOR IDENT '(' column_list ')' { LASTTABLE().addDeletor($2, $4); };
        |  CTOR '(' column_list ')'  { LASTTABLE().addCtor($3); };
        |  EXISTOR IDENT '(' column_list ')'  { LASTTABLE().addExistor($2, $4); };
        |  LOOKUP IDENT '(' column_list ')' ident_or_text { LASTTABLE().addLookup($2, $4, $6); };
        |  ALIAS '(' STRING ')'          { LASTTABLE().setAlias(TC(FString,$3));};
        |  INCLUDE '(' STRING ')'          { LASTTABLE().addInclude(TC(FString,$3));};
        |  NOUPDATE                      { LASTTABLE().addOptions(OPT_NOUPDATE); };
        |  NOREPLACE                     { LASTTABLE().addOptions(OPT_NOREPLACE); };
        |  NOINSERT                      { LASTTABLE().addOptions(OPT_NOINSERT); };
        |  NOSETSQL                      { LASTTABLE().addOptions(OPT_NOSETSQL); };
        |  NOKEYSQL                      { LASTTABLE().addOptions(OPT_NOKEYSQL); };
        |  NOWHERESQL                    { LASTTABLE().addOptions(OPT_NOWHERESQL); };
        |  NODELETE                      { LASTTABLE().addOptions(OPT_NODELETE); };
        |  NOCTOR                        { LASTTABLE().addOptions(OPT_NOCTOR); };
        |  CUSTOM                        { LASTTABLE().addOptions(OPT_CUSTOM); };
        |  CUSTOMSET                     { LASTTABLE().addOptions(OPT_CUSTOMSET); };
        |  FK ident_or_text '(' column_opt_value_list ')' fk_opt_list 
              { LASTTABLE().addForeignKey($2, $4, $6); };
        ;
selector_flags:
                                        { $$ = NEWOBJ(int(OPT_NONE)); }
      | selector_flag selector_flags   { $$ = NEWOBJ(int(TC(int,$1) | TC(int,$2))); }
      ;

selector_flag:
        LOCK_UPDATE       { $$ = NEWOBJ(int(SELECTOR_LOCK_UPDATE)); }
      | LOCK_SHARED       { $$ = NEWOBJ(int(SELECTOR_LOCK_SHARED)); }
      | LOCK_OPTIONAL     { $$ = NEWOBJ(int(SELECTOR_LOCK_OPTIONAL)); }
      ;

fk_opt_list:
                                        { $$ = NEWOBJ(int(FK_OPT_NONE)); }
      | fk_opt fk_opt_list              { $$ = NEWOBJ(int(TC(int,$1) | TC(int,$2))); }
      ;

fk_opt:
        PRIMARY                         { $$ = NEWOBJ(int(FK_OPT_PRIMARY)); }
      | ON DELETE CASCADE               { $$ = NEWOBJ(int(FK_OPT_DELETE_CASCADE)); }
      | ON DELETE RESTRICT              { $$ = NEWOBJ(int(FK_OPT_DELETE_RESTRICT)); }
      | ON UPDATE CASCADE               { $$ = NEWOBJ(int(FK_OPT_UPDATE_CASCADE)); }
      | ON UPDATE RESTRICT              { $$ = NEWOBJ(int(FK_OPT_UPDATE_RESTRICT)); }
      | ON UPDATE DELETE                { $$ = NEWOBJ(int(FK_OPT_UPDATE_DELETE)); }
      ;

create:  CREATE TABLE opt_if_not_exists table_def 
           { PC().mTables.push_back($4); }
       | CREATE DATABASE db_ident
       ;

drop:  DROP TABLE opt_if_exists table_ident ;

opt_if_exists:
          /* empty */   {}
        | IF EXISTS     {};

opt_if_not_exists:
          /* empty */   {}
        | IF NOT EXISTS     {};

grant:
        GRANT ALL PRIVILEGES ON wildcard_table_ident TO user_ident IDENTIFIED BY password ;

insert:
        INSERT INTO table_ident VALUES '(' literal_list ')' ;

replace:
        REPLACE INTO table_ident VALUES '(' literal_list ')' ;

set:
        SET IDENT_VAR EQ expr

expr:
        func_call 
      | literal
      ;

func_call:
        ident '(' opt_literal_list ')' ;

opt_literal_list:
      | literal_list
      ;

literal_list:
        literal
      | literal_list ',' literal
      ;

literal:
        HEXNUM
      | INTNUM
      | STRING
      | NULL_TOK
      | IDENT_VAR
      ;
/*
numliteral:
        HEXNUM
      | INTNUM
      ;
*/
use:     USE db_ident ;

table_ident: 
        ident
      | db_ident '.' table_ident ;

wildcard_table_ident: 
        ident
      | db_ident '.' table_ident 
      | db_ident '.' '*'
      ;

db_ident: ident ;

user_ident: 
        ident_or_text
      | ident_or_text '@' ident_or_text ;

ident:
        IDENT
      | IDENT_QUOTED
      ;

ident_or_text:
          ident
        | STRING
        ;

opt_ident_or_text:
           /* empty string */ { $$ = NEWOBJ(FString("")); }
        | ident_or_text ;

password:
        ident_or_text;

table_def:
        table_ident '(' table_element_list ')' opt_table_engine opt_default_charset 
           { 
               $$ = NEWOBJ(Table(TC(FString,$1), $3));
           };

opt_table_engine:
        | ENGINE EQ IDENT ;

opt_default_charset:
        | DEFAULT CHARSET EQ ident_or_text;

table_element_list:
        table_element
           { 
               $$ = NEWOBJ(std::vector<YYSTYPE>);
               TC(std::vector<YYSTYPE>,$$).push_back($1);
           }
      | table_element_list ',' table_element
           {
               $$ = $1;
               TC(std::vector<YYSTYPE>,$$).push_back($3);
           }
      ;

table_element:
        column_def
      | constraint_def
      ;

column_def:
        ident column_type column_option_list { $$ = NEWOBJ(TableColumn(TC(FString,$1),$2,TC(int,$3))); };

column_type:
        BIGINT '(' INTNUM ')'
            {$$ = NEWOBJ(BigIntType(TC(IntNum,$3).asInt()));}
      | BLOB
            {$$ = NEWOBJ(BlobType());}
      | BOOLEAN
            {$$ = NEWOBJ(BooleanType());}
      | CHAR '(' INTNUM ')'
            {$$ = NEWOBJ(CharType(TC(IntNum,$3).asInt()));}
      | INT '(' INTNUM ')'
            {$$ = NEWOBJ(IntType(TC(IntNum,$3).asInt()));}
      | INT
            {$$ = NEWOBJ(IntType(10));}
      | TINYINT '(' INTNUM ')'
            {$$ = NEWOBJ(TinyIntType(TC(IntNum,$3).asInt()));}
      | UNSIGNED INT '(' INTNUM ')'
            {$$ = NEWOBJ(IntType(TC(IntNum,$4).asInt(), OPT_UNSIGNED));}
      | LONGTEXT
            {$$ = NEWOBJ(LongTextType());}
      | TEXT
            {$$ = NEWOBJ(TextType());}
      | VARCHAR '(' INTNUM ')'
            {$$ = NEWOBJ(VarCharType(TC(IntNum,$3).asInt()));}
      ;

column_option_list:
                                           { $$ = NEWOBJ(int(OPT_NONE)); }
      | column_option column_option_list   { $$ = NEWOBJ(int(TC(int,$1) | TC(int,$2))); }
      ;

column_option:
        UNSIGNED          { $$ = NEWOBJ(int(OPT_UNSIGNED)); }
      | NOT NULL_TOK      { $$ = NEWOBJ(int(OPT_NOT_NULL)); }
      | AUTO_INCREMENT    { $$ = NEWOBJ(int(OPT_AUTO_INCREMENT)); }
      | DEFAULT NULL_TOK  { $$ = NEWOBJ(int(OPT_DEFAULT_NULL)); }
      | DEFAULT STRING    { $$ = NEWOBJ(int(OPT_DEFAULT)); }
      | PRIMARY KEY       { $$ = NEWOBJ(int(OPT_IN_PRIMARY_KEY)); }
      ;

constraint_def:
        PRIMARY KEY opt_ident_or_text '(' column_list ')'
          { 
            $$ = NEWOBJ(PrimaryKey(TC(FString,$3),$5));
          }
      | KEY opt_ident_or_text '(' column_list ')'  { }
      | UNIQUE opt_ident_or_text '(' column_list ')' { }
      | UNIQUE KEY opt_ident_or_text '(' column_list ')' { }
      ;

column_list:
        ident_or_text
          {
             $$ = NEWOBJ(std::vector<YYSTYPE>);
             TC(std::vector<YYSTYPE>,$$).push_back($1);
          }
      | column_list ',' ident_or_text
          {
             $$ = $1;
             TC(std::vector<YYSTYPE>,$$).push_back($3);
          }
      ;

column_opt_value_list:
        column_opt_value
                { 
                    $$ = NEWOBJ(std::vector<YYSTYPE>); 
                    TC(std::vector<YYSTYPE>,$$).push_back($1);
                }
      | column_opt_value_list ',' column_opt_value
                {
                    $$ = $1;
                    TC(std::vector<YYSTYPE>,$$).push_back($3);
                }
      ;

column_opt_value:
        IDENT_QUOTED                  { $$ = NEWOBJ(ColumnValue(TC(FString,$1), NEWOBJ(NullType()))); }
      | IDENT_QUOTED EQ literal       { $$ = NEWOBJ(ColumnValue(TC(FString,$1),$3)); }
      ;

