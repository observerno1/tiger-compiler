#ifndef _FRAME_H_
#define _FRAME_H_ 
#include "temp.h"
#include "util.h"
extern int wordSize;
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

typedef struct F_accessList_ *F_accessList;
struct F_accessList_
{
	F_access head;
	F_accessList tail;
};
//为了给有k个形式参数的函数创建一个新的栈帧，调用该函数，
// 第二个参数定义在util.h里面，描述变量是不是逃逸变量
F_frame F_newFrame(Temp_label name,U_booList formals);

Temp_label F_frame(F_frame f);  // 根据桢参数返回一个函数的label标志符？
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f,bool escape); // 逃逸变量


/****IR****/
typedef struct F_frag_ *F_frag;

struct F_frag_ {
	enum {F_stringFrag, F_procFrag} kind;
	union {
		struct {
			Temp_label label;
			string str;
		} stringg;
		struct {
			T_stm body;
			F_frame frame;
		} proc;
	} u;
};

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {
	F_frag head;
	F_fragList tail;
};

F_frag F_StringFrag(Temp_label, string);
F_frag F_ProcFrag(T_stm, F_frame);
F_fragList F_FragList(F_frag, F_fragList);
Temp_temp F_FP();
// translate 调用此函数，将F_access 转换为tree表达式，第二个是栈桢所在地址
T_exp F_Exp(F_access, T_exp);
// 调用库函数
T_exp F_externalCall(string, T_expList);
T_stm F_procEntryExit1(F_frame, T_stm);
Temp_temp F_RV();

#endif