/**********************************************************
 * Author        : lh
 * Email         : lhcoder@163.com
 * Create time   : 2017-03-06 16:18
 * Last modified : 2017-03-06 17:06
 * Filename      : env.c
 * Description   : 
 * *******************************************************/

#include <stdlib.h>
#include "util.h"
#include "env.h"

E_enventry E_VarEntry(Ty_ty ty) {
	E_enventry e = checked_malloc(sizeof(*e));
	e->kind = E_varEntry;
	e->u.var.ty = ty;
	return e;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result) {
	E_enventry e = checked_malloc(sizeof(*e));
	e->kind = E_funEntry;
	e->u.fun.formals = formals;
	e->u.fun.result = result;
	return e;
}

S_table E_base_tenv() {
	S_table t = S_empty();
	S_enter(t, S_Symbol("int"), Ty_Int());
	S_enter(t, S_Symbol("string"), Ty_String());
	return t;
}

S_table E_base_venv() {
	S_table t = S_empty();
	S_enter(t, S_Symbol("print"), E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Void()));
	S_enter(t, S_Symbol("flush"), E_FunEntry(NULL, Ty_Void()));
	S_enter(t, S_Symbol("getchar"), E_FunEntry(NULL, Ty_String()));
	S_enter(t, S_Symbol("ord"), E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Int()));
	S_enter(t, S_Symbol("chr"), E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_String()));
	S_enter(t, S_Symbol("size"), E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Int()));
	S_enter(t, S_Symbol("substring"), E_FunEntry(Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL))), Ty_String()));
	S_enter(t, S_Symbol("concat"), E_FunEntry(Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL)), Ty_String()));
	S_enter(t, S_Symbol("not"), E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_Int()));
	S_enter(t, S_Symbol("exit"), E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_Void()));
	return t;
}
