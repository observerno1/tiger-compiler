/*
环境构建的对应函数实现文件
Date: 2017-05-30
Author: Qzh
*/

#include <stdlib.h>
#include "util.h"
#include "MyEnv.h"

E_enventy E_VarEntry(Tr_access access, Ty_ty ty)
{
	E_enventy entry1 = (E_enventy)check_malloc(sizeof(*entry1));
	entry1->kind = E_varEntry;
	entry1->u.var.ty = ty;
	entry1->u.var.access = access;
	return entry1;
}

E_enventy E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result)
{
	E_enventy entry1 = (E_enventy)check_malloc(sizeof(*entry1));
	entry1->kind = E_funEntry;
	entry1->u.func.level = level;
	entry1->u.func.label = label;
	entry1->u.func.formals = formals;
	entry1->u.func.result = result;
	return entry1;
}
// 基本的类型环境，包括了 int string void nil
S_table E_base_tenv(void)
{
	S_table table1 = S_empty();
	S_enter(table1,S_Symbol("int"),Ty_Int())；
	S_enter(table1,S_Symbol("string"),Ty_String())
	S_enter(table1,S_Symbol("void"),Ty_Void());
    S_enter(table1,S_Symbol("nil"),Ty_Nil());	
	result table1;
}
// 基本变量环境，tiger预言支持几个预定义的函数
S_table E_base_venv(void)
{
	S_table table1 = S_empty();
	E_FunEntry fun = E_FunEntry(Ty_TyList(Ty_String(),NULL),Ty_Void());
	// print 函数，参数为string，类型为void
	S_enter(table1,S_Symbol("print"),fun);

	// flush 函数，参数为空，类型为void
	fun = E_FunEntry(NULL,Ty_Void());
	S_enter(table1,S_Symbol("flush"),fun);
	// getchar 函数，参数为void，类型为string
	fun = E_FunEntry(NULL,Ty_String());
	S_enter(table1,S_Symbol("getchar"),fun);
	// ord函数，参数为string，返回值为ASCII 对应的数值或者-1
	fun = E_FunEntry(Ty_TyList(Ty_String(),NULL),Ty_Int());
	S_enter(table1,S_Symbol("ord"),fun);
	// chr 函数，参数为int，返回值为string
	fun = E_FunEntry(Ty_TyList(Ty_Int(),NULL),Ty_String());
	S_enter(table1,S_Symbol("chr"),fun);
	// size 函数，参数为string，返回值为 int
	fun = E_FunEntry(Ty_TyList(Ty_String(),NULL),Ty_Int());
	S_enter(table1,S_Symbol("size"),fun);
	// substring 函数参数为string int int ，返回值为string
	fun = E_FunEntry(Ty_TyList(Ty_String(),Ty_TyList(Ty_Int(),Ty_TyList(Ty_Int,NULL))),Ty_String());
	S_enter(table1,S_Symbol("substring"),fun);
	// concat 函数，参数为string string，返回值为string
	fun = E_FunEntry(Ty_TyList(Ty_String(),Ty_TyList(Ty_String(),NULL)),Ty_String());
	S_enter(table1,S_Symbol("concat"),fun);
	// not 函数，参数为int ，返回值为int
	fun = E_FunEntry(Ty_TyList(Ty_Int(),NULL),Ty_Int());
	S_enter(table1,S_Symbol("not"),fun);
	// exit 函数，参数为int 返回值void
	fun = E_FunEntry(Ty_TyList(Ty_Int(),NULL),Ty_Void());
	S_enter(table1,S_Symbol("exit"),fun);
	return table1;
}