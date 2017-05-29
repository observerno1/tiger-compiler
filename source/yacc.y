%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "absyn.h"
#include "symbol.h" 
#include "type.h"

int yylex(void); 
void yyerror(char *s)
{
    EM_error(EM_tokPos, "%s", s);
}
A_exp absyn_root;
/*y以下分别为值环境表和类型环境表*/
S_table TotalValEnvi = NULL;
S_table TotalTypeEnvi = NULL;
/*we need a table link to remember the current environment and the past*/
/* 理解错了，其实一张S_table就可以实现环境的嵌套了
typedef struct Symbol_link_ *TableLink;
struct Symbol_link_ {S_table current, TableLink previous};
TableLink Envi = NULL;
void beginNewEnvi()
{
     if(!Envi)  //当变量环境还是空的时候，初始化
    {
        Envi = checked_malloc(sizeof(struct Symbol_link_));
        Envi->previous = NULL;
    }
    else //当环境链表早已存在的时候
    {
        TableLink temp = checked_malloc(sizeof(struct Symbol_link_));
        temp->previous = Envi;
        Envi = temp;
    }
    Envi->current = S_empty();
    S_beginScope(Envi->current);
}
void EndEnvi()
{
    S_endScope(Envi->current);
    Envi = Envi->previous;  
} */
//根据名字递归找到最底
Ty_ty FindBasisType(S_symbol name)
{
     Ty_ty temp = (Ty_ty)S_look(TotalTypeEnvi,name);
     while(temp->kind == temp->Ty_name)
     {
        temp = (Ty_ty)S_look(TotalTypeEnvi,temp->name.sym);
     }
     // 当是Ty_record Ty_int Ty_string Ty_array的时候返回
     return temp;
}
void expandTypeEnvi(S_symbol s1,A_ty ty)
{
    if(!s1 || !ty)
    {
        yyerror("error happen when expanding type environment,null pointer!\n");
        return;
    }
    switch(ty->kind)
    {
        case 0:
            // A_nameTy, we just find it int the environment
            S_symbol s2 = ty->u.name;
            // 返回值
            Ty_ty temp = FindBasisType(s2);
            // 不是基本类型，应该递归到基本类型，int string 或者array of type id，或者record类型
            S_enter(TotalTypeEnvi,s1,Ty_Name(s1,temp));
            break;
        case 1:
            // A_recordTy,
            A_fieldList tmpList = s2->u.record;
            if(tmpList == NULL) // 空的域
            {
                S_enter(TotalTypeEnvi,s1,Ty_Record(NULL)); 
            }
            else{
                // 域非空，需要每一个类型都递归到基类型
                // type a = {a1: type1, a2: type2, a3:type3, a4: a};
                // 需要注意的是首先域内各个名字不能一样，其次
            }
        case 2:
              // array of Type id, 递归到int或者string为止
              S_symbol s2 = ty->u.array;
              // 追溯到这是一个int或者
              Ty_ty temp = FindBasisType(s2); 
            // 找到最初的类型变量，ru type c = {}, type d = c,  type e = array of d,
            // 那么符号表绑定 s1, Ty_ty{d}              
            S_enter(TotalTypeEnvi,s1,Ty_Array(temp));
        default:
        /*not any one of the three kinds before, error happens*/
        yyerror("error happen when expanding type environment, type kind unexpected %d\n",ty->kind);
        break;
    }
    return;
}

%}

%union {
    int ival;
    string sval;

    int pos;

    A_var var;

    A_exp exp;
    A_expList expList;

    A_dec dec;
    A_decList decList;

    A_ty ty;

    A_field field;
    A_fieldList fieldList;

    A_fundec fundec;
    A_fundecList fundecList;
    
    A_namety namety;
    A_nametyList nametyList;

    A_efieldList efieldList;
}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%left SEMICOLON
%right THEN ELSE DOT DO OF
%right ASSIGN 
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS


%type <var> lvalue

%type <exp> program exp funcall
%type <expList> expseq paraseq

%type <dec> dec vardec
%type <decList> decs

%type <ty> ty

%type <fieldList> tyfields tyfield

%type <fundec> fundec
%type <fundecList> fundecs

%type <namety> tydec
%type <nametyList> tydecs

%type <efieldList> asseq


%start program

%%

program : exp                           {
                                         TotalTypeEnvi = S_empty(); /*nitialize an empty table*/
                                         TotalValEnvi = S_empty(); 
                                         /*we first construct the basic map from int to Ty_int and string to Ty_string*/
                                         Ty_ty intType= Ty_int();
                                         Ty_ty stringType = Ty_String();
                                         S_symbol s1 = S_Symbol("int");
                                         S_symbol s2 = S_Symbol("string");
                                         S_enter(TotalTypeEnvi,s1,intType);
                                         S_enter(TotalTypeEnvi,s2,stringType);
                                         absyn_root = $1;
                                        }
        ;

decs    : dec decs                      {$$ = A_DecList($1, $2);}
        |                               {$$ = NULL;}
        ;
                                                /*the position of token */
dec     : tydecs                        {$$ = A_TypeDec(EM_tokPos, $1);} 
        | vardec                        {$$ = $1;}
        | fundecs                       {$$ = A_FunctionDec(EM_tokPos, $1);}
        ;

tydec   : TYPE ID EQ ty                 {
                                        S_symbol temp = S_Symbol($2);
                                        // Ty可能是一个类型ID，或者一个tyfields 或者 array of 
                                        A_Namety temp = A_Namety(S_Symbol($2), $4);
                                        $$ = temp;
                                        /*扩充类型表，应该是递归的扩充*/
                                        A_ty ty = temp->ty;
                                        /*根据节点的类型来扩建*/
                                        expandTypeEnvi(temp,ty);
                                        }    
        ;

tydecs  : tydec                         {$$ = A_NametyList($1, NULL);}
        | tydec tydecs                  {$$ = A_NametyList($1, $2);}
        ;

ty      : ID                            { 
                                        /*construct an A_ty with symbol name and kind as well as position*/

                                        $$ = A_NameTy(EM_tokPos, S_Symbol($1));}
        | LBRACE tyfields RBRACE        {$$ = A_RecordTy(EM_tokPos, $2);}
        | ARRAY OF ID                   {$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}
        ;

tyfields: tyfield                       {$$ = $1;}
        |                               {$$ = NULL;}
        ;

tyfield : ID COLON ID                   {$$ = A_FieldList(A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3)), NULL);}
        | tyfield COMMA ID COLON ID     {$$ = A_FieldList(A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3)), $5);}
        ;

vardec  : VAR ID ASSIGN exp             {$$ = A_VarDec(EM_tokPos, S_Symbol($2), NULL, $4);}
        | VAR ID COLON ID ASSIGN exp    {$$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6);}
        ;

fundec  : FUNCTION ID LPAREN tyfields RPAREN EQ exp             {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL, $7);}
        | FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp    {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);}
        ;

fundecs : fundec                        {$$ = A_FundecList($1, NULL);}
        | fundec fundecs                {$$ = A_FundecList($1, $2);}
        ;

lvalue  : ID                            {$$ = A_SimpleVar(EM_tokPos,S_Symbol($1));}
        | lvalue DOT ID                 {$$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));}
        | lvalue LBRACK exp RBRACK      {$$ = A_SubscriptVar(EM_tokPos, $1, $3);}
        | ID LBRACK exp RBRACK          {$$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos, S_Symbol($1)), $3);}
        ;   

exp : lvalue                            {$$ = A_VarExp(EM_tokPos, $1);}
    | NIL                               {$$ = A_NilExp(EM_tokPos);}
    | LPAREN expseq RPAREN              {$$ = A_SeqExp(EM_tokPos, $2);}
    | LPAREN RPAREN                     {$$ = A_SeqExp(EM_tokPos, NULL);}
    | INT                               {$$ = A_IntExp(EM_tokPos, $1);}
    | STRING                            {$$ = A_StringExp(EM_tokPos, $1);}

    | MINUS exp %prec UMINUS            {$$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}

    | funcall                           {$$ = $1;}

    | exp PLUS exp                      {$$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);}
    | exp MINUS exp                     {$$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
    | exp TIMES exp                     {$$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
    | exp DIVIDE exp                    {$$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3);}

    | exp EQ exp                        {$$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);}
    | exp NEQ exp                       {$$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
    | exp LT exp                        {$$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);}
    | exp LE exp                        {$$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);}
    | exp GT exp                        {$$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);}
    | exp GE exp                        {$$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);}

    | exp AND exp                       {$$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0));}
    | exp OR exp                        {$$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos, 1), $3);}

    | ID LBRACE RBRACE                  {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), NULL);}
    | ID LBRACE asseq RBRACE            {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);}

    | ID LBRACK exp RBRACK OF exp       {$$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);}

    | lvalue ASSIGN exp                 {$$ = A_AssignExp(EM_tokPos, $1, $3);}

    | IF exp THEN exp                   {$$ = A_IfExp(EM_tokPos, $2, $4, NULL);}
    | IF exp THEN exp ELSE exp          {$$ = A_IfExp(EM_tokPos, $2, $4, $6);}

    | WHILE exp DO exp                  {$$ = A_WhileExp(EM_tokPos, $2, $4);}

    | FOR ID ASSIGN exp TO exp DO exp   {$$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);}

    | BREAK                             {$$ = A_BreakExp(EM_tokPos);}

    | LET decs IN END                   {
                                            S_beginScope(TotalEnvi);
                                            $$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, NULL));
                                            S_endScope(TotalEnvi);
                                        }
    | LET decs IN expseq END            {  
                                            S_beginScope(TotalEnvi);
                                            $$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));
                                            S_endScope(TotalEnvi);
                                        }
    ;

expseq  : exp                           {$$ = A_ExpList($1, NULL);}
        | exp SEMICOLON expseq          {$$ = A_ExpList($1, $3);}
        ;

funcall : ID LPAREN RPAREN              {$$ = A_CallExp(EM_tokPos, S_Symbol($1), NULL);}
        | ID LPAREN paraseq RPAREN      {$$ = A_CallExp(EM_tokPos, S_Symbol($1), $3);}
        ;

paraseq : exp                           {$$ = A_ExpList($1, NULL);}
        | exp COMMA paraseq             {$$ = A_ExpList($1, $3);}
        ;

asseq   : ID EQ exp                     {$$ = A_EfieldList(A_Efield(S_Symbol($1), $3), NULL);}
        | ID EQ exp COMMA asseq         {$$ = A_EfieldList(A_Efield(S_Symbol($1), $3), $5);}
        ;