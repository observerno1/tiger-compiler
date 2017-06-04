/*
提供了frame.c的抽象实现
*/
#include "frame.h"

// 每个字4个bytes
const int wordSize = 4;

struct F_frame_
{
	Temp_label name;
	F_accessList formals;
	int Local_varCount;
}
// 得到一个变量的具体访问地址
// 那么
struct F_access_{
	enum {inFrame,inReg} kind;
	union{
		int offest;
		Temp_temp RegIndex;
	}u;
}

// 抽象的实现，并不管具体的
F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame f = checked_malloc(sizeof(*f));
	f->name = name;
	f->formals = makeFormalAccessList(formals);
	f->local_count = 0;
	return f;
}

F_access F_allocLocal(F_frame f, bool escape) {
	/* give a not-paras-var alloc space just is in let-in-end-exp decl a var */
	f->local_count++;
	if (escape) {
		return InFrame(-1 * F_wordSize * f->local_count);
	}
	return InReg(Temp_newtemp());
}
// 返回桢内变量列表
F_accessList F_formals(F_frame f)
{
	if(!f)
		return NULL;
	return f->formals;
}

Temp_label F_name(F_frame f)
{
	if(!f)
		return NULL;
	return f->name;
}
// 按照书上的
 F_access InFrame(int offs) {
	F_access a = checked_malloc(sizeof(*a));
	a->kind = inFrame;
	a->u.offs = offs;
	return a;
}

F_access InReg(Temp_temp t) {
	F_access a = checked_malloc(sizeof(*a));
	a->kind = inReg;
	a->u.reg = t;
	return a;
}
// F_access F_allocLocal(F_frame f,bool escape)
// {
// 	// 逃逸变量
// 	assert(f);
// 	if(escape)
// 	{
// 		f->Local_varCount++;
// 		F_access tmp = checked_malloc(sizeof(*tmp));
// 		tmp->kind = inFrame;
// 		// 假设内存地址从大到小增长
// 		tmp->u.offest = wordSize * (f->Local_varCount -1);
// 		return tmp;
// 	}
// 	else
// 	{
// 		f->Local_varCount++;
// 		F_access tmp = checked_malloc(sizeof(*tmp));
// 		tmp->kind = inReg;
// 		f->u.RegIndex = Temp_newtemp(); // 生成临时寄存器的
// 		return tmp;

// 	}
// }

static F_accessList makeFormalAccessList(U_boolList formals) {
	U_boolList fmls = formals;
	F_accessList head = NULL, tail = NULL;
	int i = 0;
	for (; fmls; fmls = fmls->tail, i++) {
		F_access ac = NULL;
		if (!fmls->head) {  // 非逃逸变量
			ac = InReg(Temp_newtemp());
		} else {
			/*keep a space for return*/
			ac = InFrame((i/* + 1*/) * F_wordSize);
		}
		if (head) {
			tail->tail = F_AccessList(ac, NULL);
			tail = tail->tail;
		} else {
			head = F_AccessList(ac, NULL);
			tail = head;
		}
	}
	return head;
}
// 描述字符串的片段，由所在层的label和本身组成，前者实际上是一个symbol指针
F_frag F_StringFrag(Temp_label label, string str) {
	F_frag strfrag = checked_malloc(sizeof(*strfrag));
	strfrag->kind = F_stringFrag;  // 类型
	strfrag->u.stringg.label = label; 
	strfrag->u.stringg.str = str; // char 指针
	return strfrag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
	F_frag pfrag = checked_malloc(sizeof(*pfrag));
	pfrag->kind = F_procFrag;
	pfrag->u.proc.body = body;
	pfrag->u.proc.frame = frame;
	return pfrag;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
	F_fragList fl = checked_malloc(sizeof(*fl));
	fl->head = head;
	fl->tail = tail;
	return fl;
}
// 帧指针寄存器，那么调用函数的时候需要保存之
static Temp_temp fp = NULL;
Temp_temp F_FP(void) { /* frame-pointer */
	if (!fp) {
		fp = Temp_newtemp();
	}
	return fp;
}
// 转化为树结构，
T_exp F_Exp(F_access access, T_exp framePtr){ 
	if (access->kind == inFrame) {
		T_exp e = T_Mem(T_Binop(T_plus, framePtr, T_Const(access->u.offs)));
		return e;
	} else {
		return T_Temp(access->u.reg);
	}
}

T_exp F_externalCall(string str, T_expList args) {
	return T_Call(T_Name(Temp_namedlabel(str)), args);
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
	return stm;
}