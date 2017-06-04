#include "translate.h"
#include "frame.h"
#include "tree.h"
// 填充跳转地址的表
struct patchList_ {
    Temp_label *head; 
    patchList tail; 
};
struct Tr_exp_
{
	/* data */
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    }u;
};
struct Cx { 
	patchList trues; 
	patchList falses; 
	T_stm stm; };

struct Tr_exp_ {
    enum {Tr_ex, Tr_nx, Tr_cx} kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    }u;
};

static Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp ret = checked_malloc(sizeof(*ret));
    ret->kind = Tr_ex;
    ret->u.ex = ex;
    return ret;
}

static Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp ret = checked_malloc(sizeof(*ret));
    ret->kind = Tr_nx;
    ret->u.nx = nx;
    return ret;
}

struct Tr_level_{
    Tr_level parent;  // 上一级层次
    Temp_label name;  // 这一层名字
    F_frame frame;   // 所在的栈桢
    Tr_accessList formals; //暂时不知道这个参数用来干嘛
}
// 处理一个位于lev层的变量的时候，返回该结果，包括了所在静态连信息，还有在桢内的信息
struct Tr_access_
{
    Tr_level level;
    F_access access;
}
// 在当前静态链的情况下新建一个静态连
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals)
{
    Tr_level tmp = checked_malloc(sizeof(*tmp));
    tmp->parent = parent;
    tmp->name = name;
    // 多增加一个，因为新建一层需要保存上一层的静态链的信息，需要多一个faccess位置
    tmp->frame = F_newFrame(name,U_boolList(true,formals));
    tmp->formals = accessListFromLevel(tmp);
}
//
static Tr_level outMost = NULL;
Tr_level Tr_outermost(void)
{
    if(!outMost)
    {
        outMost = Tr_newLevel(NULL,Temp_newlabel(),NULL);
    }
    return outMost;
}
// tr_access 的构造函数
Tr_access Tr_Access(Tr_level level,F_access access)
{
    if(!level||!access)
    {
        return NULL;
    }
    Tr_access tmp = checked_malloc(sizeof(*tmp));
    tmp->level = level;
    tmp->access = access;
    return tmp;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail)
{
    Tr_accessList tmp = checked_malloc(sizeof(*tmp));
    tmp->head = head;
    tmp->tail = tail;
    // 
}

// 从一个静态链信息中获取该静态链的访问参数信息
Tr_accessList accessListFromLevel(Tr_level level)
{
    if(level == NULL) return NULL;
   assert(level->frame);

   F_accessList formals = F_formals(level->frame);
   //第一个位置保存静态连信息，跳过
   formals = formals->tail;

   Tr_accessList list,temp;
   list = checked_malloc(sizeof(*list));
   list->head = NULL;
   list->tail = NULL;
   temp = list;

   while(formals) {
        temp->head = Tr_makeAccess(level, formals->head);
        if(formals->tail)
            temp->tail = checked_malloc(sizeof(*temp->tail));
        else
            temp->tail = NULL;
        formals = formals->tail;
        temp = temp->tail;
   }
   return list;
}
// 申请一个局部变量
Tr_access Tr_allocLocal(Tr_level level, bool escape)
{
  assert(level);
  F_frame frame = level->frame;
  F_access f_access = F_allocLocal(frame,escape);
  Tr_access tr_access = checked_malloc(sizeof(*tr_access));
  tr_access->access = f_access;
  tr_access->level = level;
  return tr_access;
}

// 条件转移语句结构
Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
    Tr_exp ret = checked_malloc(sizeof(*ret));
    ret->kind = Tr_cx;
    ret->u.cx.trues = trues;
    ret->u.cx.falses = falses;
    ret->u.cx.stm = stm;
    return ret;
}
patchList PatchList(Temp_label *head, patchList tail) {
    patchList tmp = checked_malloc(sizeof(*ret));
    tmp->head = head;
    tmp->tail = tail;
    return tmp;
}