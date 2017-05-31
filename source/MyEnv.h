#ifndef _ENV_H
#define _ENV_H
#include "symbol.h"
#include "types.h"
typedef struct E_enventy_ *E_enventy;
struct E_enventy_
{
	enum {E_VarEntry, E_FunEntry} kind;
	union{
		struct { Ty_ty ty} var;
		struct { Ty_TyList formals, Ty_ty result} func;
	}u;
};
/*cnnstruction function for a var entry which used to fill in value environment*/
E_enventy E_VarEntry(Ty_ty ty); 
// 函数变量的值
E_enventy E_FunEntry(Ty_TyList formals, Ty_ty result);

// 类型环境
S_table E_base_tenv(void);

// 值环境
S_table E_base_venv(void);

#endif