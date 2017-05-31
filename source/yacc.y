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
typedef struct Null_A_nametyList *NullTypeList;
struct Null_A_nametyList { 
    A_namety node, 
    NullTypeList next;
};
NullTypeList WaitTypeList =  NULL;
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
// 如果一个变量声明类型尚未找到，可能是在后续中，还没出现，加入待补充队列中
void addItem(A_namety item)
{
    NullTypeList tail = (NullTypeList)checked_malloc(sizeof(*WaitTypeList));
    tail->next = NULL;
    if(WaitTypeList == NULL)
    {
        WaitTypeList = tail;
    }
    else
    {
        NullTypeList temp;
        temp = NullTypeList;
        while(temp!=NULL && temp->next)
        {
            temp = temp->next;
        }
        temp->next = tail;
    }
}
// 一旦连续的声明结束之后，执行清理操作
void clearItem()
{
    NullTypeList temp = WaitTypeList;
    while(temp!=NULL)
    {
        temp = WaitTypeList->next;
        free(WaitTypeList);
        WaitTypeList = temp;
    }
}
// 链表开头最好还是放一个 哑节点 但这里没有
void removeOneItem(A_namety item)
{
    if(WaitTypeList == NULL)
        return;
    NullTypeList tmp1 = WaitTypeList;
    NullTypeList tmp2 = WaitTypeList;
    if(WaitTypeList->node == item)
    {
        WaitTypeList = WaitTypeList->next;
        free(tmp1);
        return;
    }
    whle(tmp1 != NULL){
        tmp2 = tmp1->next;
        if(tmp2 != NULL && tmp2->node == item)
        {
            tmp->next = tmp2->next;
            free(tmp2);
            break;
        }
        tmp1 = tmp2;
    }
}
//根据名字递归找到最底
Ty_ty FindBasisType(S_symbol name)
{
     Ty_ty temp = (Ty_ty)S_look(TotalTypeEnvi,name);
     while(temp != NULL && temp->kind == temp->Ty_name)
     {
        temp = temp->name.ty;
     }
     // 当是Ty_record Ty_int Ty_string Ty_array的时候返回
     return temp;
}
// A_namety
void expandTypeEnvi(A_namety t1)
{
    S_symbol s1;
    A_ty ty;
    s1 = t1->name;
    ty = t1->ty;
    // 声明的类型已经存在
    if(S_look(TotalTypeEnvi,s1))
    {
        yyerror("type declaartion with exsiting types! %s\n",s1);
        return;
    }
    if(!s1 || !ty)
    {
        yyerror("error happen when expanding type environment,null pointer!\n");
        return;
    }
    bool flag = true;
    switch(ty->kind)
    {
        case 0:
            // A_nameTy, we just find it int the environment
            S_symbol s2 = ty->u.name;
            // 返回值
            Ty_ty temp = (Ty_ty)S_look(TotalTypeEnvi,s2);
            if(temp == NULL) 
            {
                addItem(t1);
                return;
            }
            // 不是基本类型，应该递归到基本类型，int string 或者array of type id，或者record类型
            S_enter(TotalTypeEnvi,s1,Ty_Name(s1,temp));
            break;
        case 1:
            // A_recordTy,
            bool selfRecursive = false;
            A_fieldList tmpList = s2->u.record;
            Ty_fieldList typeList =  NULL;
            if(tmpList == NULL) // 空的域
            {
                S_enter(TotalTypeEnvi,s1,Ty_Name(s1,Ty_Record(NULL)); 
            }
            else{
                // 域非空，需要每一个类型都说明，但是不递归到基本类型
                // type a = {a1: type1, a2: type2, a3:type3, a4: a};
                // 需要注意的是首先域内各个名字不能一样，其次,k可能出现循环递归
                // 对于普通的域，应该采用作用域+.+name 加入绑定表里面,免得覆盖之前加入的类型如 type a1:int
                // 对于递归类型的呢
                A_field head = tmpList->head;
                S_symbol symIDname, symTypeName;
                S_symbol FullSym = NULL;
                S_beginScope(TotalTypeEnvi);
                while(head != NULL)
                {
                    symIDname = head->name;
                    symTypeName = head->typ;
                    string str = (string)checked_malloc(sizeof(char)*(strlen(s1->name)+strlen(symIDname->name)+2));
                    // 一般来说内存能够申请到
                    strcpy(str,s1->name);
                    strcat(str,".");
                    // copy
                    strcat(str,symIDname->name);
                    FullSym = S_Symbol(str);
                    Ty_ty tmpField = S_look(TotalTypeEnvi,FullSym); // 查找存在与否
                    if(!tmpField) // 域名内变量冲突
                    {
                        yyerror("type confict witth existing type in record field at position %d\n",head->pos);
                        // 这一行不分析了，返回
                        S_endScope(TotalTypeEnvi);
                        return;
                    }
                    // 自我递归定义
                    if(symTypeName == s1)
                    {
                        selfRecursive = true;
                    }
                    // 变量未加入，考察右边的类型是否存在
                    Ty_ty tmpty1 = (Ty_ty)S_look(TotalTypeEnvi,symTypeName);
                    if(tmpty1 == NULL && symTypeName != s1) // 变量类型尚且不存在
                    {
                        addItem(t1);
                        flag = false;
                        S_endScope(TotalTypeEnvi);
                        return; // 依赖于其他未知变量，加入队列，先不管
                    }
                    else{
                        // 标记一下
                        S_enter(TotalTypeEnvi,FullSym,Ty_Name(FullSym,NULL)); // 解决域名内冲突
                        Ty_field f1 = Ty_Field(symIDname,tmpty1);
                        typeList = Ty_FieldList(f1,typeList);
                    } 
                    head = head->tail; 
                }
                S_endScope(TotalTypeEnvi);
                // 域内所有类型都找到了
                if(flag)
                {
                    //名字+对应链表结构
                    Ty_Name temp2 = Ty_Name(s1,Ty_Record(typeList);
                    S_enter(TotalTypeEnvi,s1,temp2);
                    if(selfRecursive)
                    {// 出现了自我递归定义
                        Ty_field nodeField;
                        while(typeList!=NULL){
                            nodeField = typeList->head;
                            if(nodeField->ty == NULL)
                            {
                                nodeField->ty = temp2;
                            }
                            typeList = typeList->tail;
                        }
                    }
                }
            }
        case 2:
              // array of Type id, 定义的时候不递归，比较到时候递归比较
              S_symbol s2 = ty->u.array;
              // 追溯到这是一个int或者
              Ty_ty temp = (Ty_ty)S_look(TotalTypeEnvi,s2);
             if(temp == NULL) 
             {
                addItem(t1);
                return;
             }
            // 找到最初的类型变量，ru type c = {}, type d = c,  type e = array of d,
            // 那么符号表绑定 s1, Ty_ty{d}              
            S_enter(TotalTypeEnvi,s1,Ty_Name(s1,Ty_Array(temp));
        default:
        /*not any one of the three kinds before, error happens*/
        yyerror("error happen when expanding type environment, type kind unexpected %d\n",ty->kind);
        return; 
    }
    removeOneItem(t1); // 尝试从未解决变量队列中删除
    NullTypeList w1 = WaitTypeList;
    NullTypeList w2 = WaitTypeList;
    while(w1 =NULL) // 每一次成功的声明都试图解决可能存在递归声明的类型链表
    {
        w1 = WaitTypeList->next;
        expandTypeEnvi(w2);
        w2 = w1;
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
                                        S_symbol tempSym = S_Symbol($2);
                                        // Ty可能是一个类型ID，或者一个tyfields 或者 array of 
                                        A_namety temp = A_Namety(S_Symbol($2), $4);
                                        $$ = temp;
                                        /*扩充类型表，应该是递归的扩充*/
                                        A_ty ty = temp->ty;
                                        /*根据节点的类型来扩建*/
                                        expandTypeEnvi(temp);
                                        }    
        ;

tydecs  : tydec                         {$$ = A_NametyList($1, NULL);}
        | tydec tydecs                  {
                                            $$ = A_NametyList($1, $2);
                                            // 一连串声明结束之后，检查是不是还有类型未知的，如果是则出错
                                            if(WaitTypeList !=  NULL)
                                            { // 还有未知类型变量，报错
                                                yyerror("type declaartion error, exsiting loop recursive declaartion%d\n");
                                            }
                                        }
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
        | ID COLON ID COMMA tyfield     {$$ = A_FieldList(A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3)), $5);}
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
                                            S_beginScope(TotalTypeEnvi);
                                            $$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, NULL));
                                            S_endScope(TotalTypeEnvi);
                                        }
    | LET decs IN expseq END            {  
                                            S_beginScope(TotalTypeEnvi);
                                            $$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));
                                            S_endScope(TotalTypeEnvi);
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