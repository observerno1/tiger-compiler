/*
 * absyn.h - Abstract Syntax Header (Chapter 4)
 *
 * All types and functions declared in this header file begin with "A_"
 * Linked list types end with "..list"
 */

#ifndef ABSYN_H
#define ABSYN_H

#include "util.h"
#include "symbol.h"

/* Type Definitions */
//struct declaration

typedef int A_pos;

typedef struct A_var_ *A_var;

typedef struct A_exp_ *A_exp;
typedef struct A_expList_ *A_expList;

typedef struct A_dec_ *A_dec;
typedef struct A_decList_ *A_decList;

typedef struct A_ty_ *A_ty;

typedef struct A_field_ *A_field;
typedef struct A_fieldList_ *A_fieldList;

typedef struct A_fundec_ *A_fundec;
typedef struct A_fundecList_ *A_fundecList;

typedef struct A_namety_ *A_namety;
typedef struct A_nametyList_ *A_nametyList;

typedef struct A_efield_ *A_efield;
typedef struct A_efieldList_ *A_efieldList;

typedef enum {
    A_plusOp,
    A_minusOp,
    A_timesOp,
    A_divideOp,
    A_eqOp,
    A_neqOp,
    A_ltOp,
    A_leOp,
    A_gtOp,
    A_geOp
} A_oper;

// ID和STRING都要进入符号表，所以都是S_symbol类型

// 变量定义，对应的是lvalue的展开式
struct A_var_{
    A_pos pos;
    enum {
        A_simpleVar,        //lvalue -> ID
        A_fieldVar,         //lvalue -> lvalue.ID
        A_subscriptVar      //lvalue -> lvalue.[exp]
    } kind;
	union {
        S_symbol simple;    //simple记载了ID
        struct {
            A_var var;      //指向一个lvalue
            S_symbol sym;   //sym记载了ID
        } field;
        struct {
            A_var var;      //指向一个lvalue
            A_exp exp;      //指向一个exp
        } subscript;
    } u;
};

// 表达式定义，对应的是exp的展开式
struct A_exp_{
    A_pos pos;
    enum {
        A_varExp,            //exp -> lvalue
        A_nilExp,
        A_intExp,
        A_stringExp,
        A_callExp,           //exp -> funcall
        A_opExp,
        A_recordExp,         //exp -> ID {asseq}
        A_seqExp,            //exp -> (expseq)
        A_assignExp,
        A_ifExp,
        A_whileExp,
        A_forExp,
        A_breakExp,
        A_letExp,
        A_arrayExp           //exp -> ID [exp] OF exp
    } kind;
    union {
        A_var var;
        /* nil; - needs only the pos */
        int intt;
        string stringg;
        struct {
            S_symbol func;    //函数名
            A_expList args;   //函数参数列表，指向一个expList
        } call;
        struct {
            A_oper oper;
            A_exp left;
            A_exp right;
        } op;
        struct {
            S_symbol typ;           //record的ID
            A_efieldList fields;    //指向asseq的内容
        } record;
        A_expList seq;
        struct {
            A_var var;
            A_exp exp;
        } assign;
        struct {
            A_exp test,then, elsee;
        } iff; /* elsee is optional */
        struct {
            A_exp test, body;
        } whilee;
        struct {
            S_symbol var;
            A_exp lo,hi,body;
            bool escape;
        } forr;
        /* breakk; - need only the pos */
        struct {
            A_decList decs;
            A_exp body;
        } let;
        struct {
            S_symbol typ;       //数组的ID
            A_exp size;         //数组的大小，由一个exp指定
            A_exp init;         //元素初始值，由一个exp指定
        } array;
    } u;
};

// 声明定义
struct A_dec_ {
    A_pos pos;
    enum {
        A_functionDec,  //函数声明
        A_varDec,       //变量声明
        A_typeDec       //类型声明
    } kind;
    union {
        A_fundecList function;  //函数
	    /* escape may change after the initial declaration */
	    struct {
            S_symbol var;       //变量的ID
            S_symbol typ;       //变量的类型名称
            A_exp init;         //变量的值，由一个exp指定
            bool escape;
        } var;
	    A_nametyList type;
    } u;
};

// 类型定义
struct A_ty_ {
    A_pos pos;
    enum {
        A_nameTy,       //普通定义
        A_recordTy,     //记录定义
        A_arrayTy       //数组定义
    } kind;
    union {
        S_symbol name;          //ty -> ID 中的ID（新类型名）
        A_fieldList record;     //ty -> {tyfields} 中的tyfields
        S_symbol array;         //数组名ID
    } u;
};

//链表和其上的节点

//这个是类型声明节点，对应的产生式是tydec -> TYPE ID EQ ty
struct A_field_ {
    S_symbol name;      //新类型ID
    S_symbol typ;       //旧类型ID
    A_pos pos;
    bool escape;
};

//这个是类型声明链表，对应的产生式是tydecs -> tydec tydecs
struct A_fieldList_ {
    A_field head;           //head一定有
    A_fieldList tail;       //tail不一定有
};

//这个是exp链表，对应的产生式是expseq -> exp SEMICOLON expseq
struct A_expList_ {
    A_exp head;
    A_expList tail;
};

//一个函数定义节点
struct A_fundec_ {
    A_pos pos;
    S_symbol name;          //函数ID
    A_fieldList params;     //参数
    S_symbol result;        //返回值类型ID
    A_exp body;             //函数体，由一个exp指定
};

//函数定义链表
struct A_fundecList_ {
    A_fundec head;
    A_fundecList tail;
};

//这个是一串dec，对应decs -> dec decs
struct A_decList_ {
    A_dec head;
    A_decList tail;
};

//一个类型定义，名字是name，类型是ty（长成ty模样）
//对应的产生式是tyfield -> ID COLON ID
struct A_namety_ {
    S_symbol name;
    A_ty ty;
};

//类型定义的链表
struct A_nametyList_ {
    A_namety head;
    A_nametyList tail;
};

//在record中这个会被用到，对应产生式为asseq -> ID EQ exp
struct A_efield_ {
    S_symbol name;      //新增记录域的ID
    A_exp exp;          //新增记录域的值，由exp指定
};

//asseq的链表
struct A_efieldList_ {
    A_efield head;
    A_efieldList tail;
};


/* Function Prototypes */
//construct functions

A_var A_SimpleVar(A_pos pos, S_symbol sym);
A_var A_FieldVar(A_pos pos, A_var var, S_symbol sym);
A_var A_SubscriptVar(A_pos pos, A_var var, A_exp exp);

A_exp A_VarExp(A_pos pos, A_var var);
A_exp A_NilExp(A_pos pos);
A_exp A_IntExp(A_pos pos, int i);
A_exp A_StringExp(A_pos pos, string s);
A_exp A_CallExp(A_pos pos, S_symbol func, A_expList args);
A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right);
A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields);
A_exp A_SeqExp(A_pos pos, A_expList seq);
A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp);
A_exp A_IfExp(A_pos pos, A_exp test, A_exp then, A_exp elsee);
A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body);
A_exp A_ForExp(A_pos pos, S_symbol var, A_exp lo, A_exp hi, A_exp body);
A_exp A_BreakExp(A_pos pos);
A_exp A_LetExp(A_pos pos, A_decList decs, A_exp body);
A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init);
A_expList A_ExpList(A_exp head, A_expList tail);

A_dec A_FunctionDec(A_pos pos, A_fundecList function);
A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init);
A_dec A_TypeDec(A_pos pos, A_nametyList type);
A_decList A_DecList(A_dec head, A_decList tail);

A_ty A_NameTy(A_pos pos, S_symbol name);
A_ty A_RecordTy(A_pos pos, A_fieldList record);
A_ty A_ArrayTy(A_pos pos, S_symbol array);

A_field A_Field(A_pos pos, S_symbol name, S_symbol typ);
A_fieldList A_FieldList(A_field head, A_fieldList tail);

A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result, A_exp body);
A_fundecList A_FundecList(A_fundec head, A_fundecList tail);

A_namety A_Namety(S_symbol name, A_ty ty);
A_nametyList A_NametyList(A_namety head, A_nametyList tail);

A_efield A_Efield(S_symbol name, A_exp exp);
A_efieldList A_EfieldList(A_efield head, A_efieldList tail);

#endif
