#ifndef _TRANSLATE_H
#define _TRANSLATE_H

#include "frame.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;
typedef struct Tr_expList_ *Tr_expList;
typedef struct patchList_ *patchList;

struct Tr_expList_ {Tr_exp head; Tr_expList tail;};
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

struct Tr_accessList_ { 
	Tr_access head; 
	Tr_accessList tail; 
};
Tr_accessList Tr_AccessList(
	Tr_access head, 
	Tr_accessList tail)
;

Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);
Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level, bool escape);

void doPatch(patchList, Temp_label);
// 在表达式链表前后加入
void Tr_ExpList_append(Tr_expList list, Tr_exp expr);

Tr_exp Tr_seqExp(Tr_expList list);
Tr_exp Tr_simpleVar(Tr_access, Tr_level);
Tr_exp Tr_fieldVar(Tr_exp record, int index);
Tr_exp Tr_subscriptVar(Tr_exp array, Tr_exp index);

Tr_exp Tr_voidExp();
Tr_exp Tr_nilExp();
Tr_exp Tr_intExp(int);
Tr_exp Tr_stringExp(string str, Tr_level level);
Tr_exp Tr_callExp(Temp_label fun, Tr_level called, Tr_level defined, Tr_expList args);
Tr_exp Tr_opExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_recordExp(int n/*field cnt, n*W */, Tr_expList fields);
Tr_exp Tr_ifExp(Tr_exp ifexp, Tr_exp thenexp, Tr_exp elseexp);
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label ldone);
Tr_exp Tr_breakExp(Temp_label lbreak);
Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label ldone);

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp val);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);

Tr_exp Tr_varDec(Tr_access, Tr_exp);
Tr_exp Tr_funDec(Tr_level level, Tr_exp body);
Tr_exp Tr_typeDec();

F_fragList Tr_getResult(void);
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);
extern F_fragList gFrags; //??


#endif