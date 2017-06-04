##ifndef _SEGMENT_H_
#define _SEGMENT_H_ 
#include "symbol.h"
#include "absyn.h"
#include "translate.h"
#include "errormsg.h"
#include "types.h"

// typedef void* Tr_exp; in translate.h
struct expty 
{
	Tr_exp  exp;
	Tr_ty ty;
};

struct expty expTy(Tr_exp exp, Ty_ty ty);

// type-checking and translate into intermediate code
struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void transDec(S_table venv, S_table tenv, A_dec d);
void SEM_transProg(A_exp exp);
// 根据类型声明找到一个类型非 Ty_namety的类型，最基本的类型
Ty_ty actual_ty(Ty_ty t);
Ty_ty transTy(S_table tenv, A_ty a);
// 从参数中构造一个全部是逃逸变量的
U_boolList makeEscapeFormalList(A_fieldList p);

#endif
