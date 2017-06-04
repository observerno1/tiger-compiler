#include "translate.h"
#include "frame.h"
#include "tree.h"

F_fragList gFrags = NULL;
Tr_exp Tr_ifWithNoElse(Tr_exp cond, Tr_exp then);
Tr_exp Tr_ifWithElse(Tr_exp cond, Tr_exp then, Tr_exp elseexp);

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
//找到第二个静态连到第一个静态连的树表达式
static Tr_exp Tr_StaticLink(Tr_level funLevel, Tr_level level)
{
    T_exp addr = T_Temp(F_FP());
    /* Follow static links until we reach level of defintion */
    while (level != funLevel->parent) {
        /* Static link is the first frame formal */
        F_access staticLink = level->frame->formals->head;
        addr = F_Exp(staticLink, addr);
        level = level->parent;
    }
    return Tr_Ex(addr);
}
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
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail)
{   
    Tr_expList temp = checked_malloc(sizeof(*temp));
    temp->head = head;
    temp->tail = tail;
}
void Tr_ExpList_append(Tr_expList list, Tr_exp expr)
{
    if(!list)
        return;
    else
    {
        Tr_expList tmp = list;
        while(tmp->tail)
        {
            tmp = tmp->tail;
        }
        tmp->tail = Tr_ExpList(expr,NULL);
    }
}
Tr_exp Tr_seqExp(Tr_expList list)
{
    T_exp result = unEx(list->head->expr);
    Tr_node p;
    for (p = list->head->next; p; p = p->next)
        result = T_Eseq(T_Exp(unEx(p->expr)), result);
    return Tr_Ex(result);
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
        temp->head = Tr_Access(level, formals->head);
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
//Tr_exp Ex的反构造函数.
T_exp unEx(Tr_exp e) {

    switch (e->kind) {
        case Tr_ex: return e->u.ex;
        case Tr_nx: return T_Eseq(e->u.nx,T_Const(0));
        case Tr_cx: {
           // 条件语句,如果为真返回1，为假返回0
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues,t);
            doPatch(e->u.cx.falses,f);
            return T_Eseq(T_Move(T_Temp(r),T_Const(1)),
                    T_Eseq(e->u.cx.stm,
                     T_Eseq(T_Label(f),
                      T_Eseq(T_Move(T_Temp(r),T_Const(0)),
                        T_Eseq(T_Label(t),T_Temp(r))))));
        }
       
        assert(0);
    }
      
}
static T_stm unNx(Tr_exp e)
{
    switch(e->kind) {
        case Tr_ex:
            return T_Exp(e->u.ex);
        case Tr_nx:
            return e->u.nx;
        case Tr_cx:
        {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Exp(T_Eseq(T_Move(T_Temp(r), T_Const(1)),
                T_Eseq(e->u.cx.stm,
                    T_Eseq(T_Label(f), 
                        T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                            T_Eseq(T_Label(t), T_Temp(r)))))));
        }
        default:
        {
            assert(0);
        }
    }
    return NULL;
}
// 条件语句
static struct Cx unCx(Tr_exp e)
{
    switch(e->kind) {
        case Tr_ex:
        {
            struct Cx cx;
            /* If comparison yields true then the expression was false (compares equal
             * to zero) so we jump to false label. */
            cx.stm = T_Cjump(T_eq, e->u.ex, T_Const(0), NULL, NULL);
            cx.trues = PatchList(&(cx.stm->u.CJUMP.false), NULL);
            cx.falses = PatchList(&(cx.stm->u.CJUMP.true), NULL);
            return cx;
        }
        case Tr_nx:
        {
            assert(0); // Should never occur
        }
        case Tr_cx:
        {
            return e->u.cx;
        }
        default:
        {
            assert(0);
        }
    }
}

// 填充一个真或者假的标号表，用一个label填充
void doPatch(patchList tList, Temp_label label) {
    for(; tList; tList = tList->tail)
        *(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second) {
    if(!first) return second;
    for (; first->tail; first=first->tail); /* go to the end of list*/
    first->tail = second;
    return first;
}

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level)
{
    T_exp fp = T_Temp(F_FP()); // 后者返回一个含有新的临时寄存器的表达式子
    while (level != access->level) {
        /* Static link is the first frame formal */
        F_access staticLink = level->frame->formals->head; //第一个应该存放着静态连
        fp = F_Exp(staticLink, fp);
        level = level->parent;
    }
    // 变量不在自己的层内使用的时候，追踪静态链
    T_exp var = F_Exp(acc->access, fp);     // 转化为 Mem(binop,temp fp,const k)的结构
    return Tr_Ex(var);
}
// 记录变量
Tr_exp Tr_fieldVar(Tr_exp record, int index)
{
 return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(record), T_Const(fieldOffset * wordSize))));
}

// 数组变量
Tr_exp Tr_subscriptVar(Tr_exp arrayBase, Tr_exp index)
{
    return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(arrayBase), 
                T_Binop(T_mul, unEx(index), T_Const(F_WORD_SIZE)))));
}

Tr_exp Tr_nilExp() {
    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_voidExp(){
    return Tr_Nx(T_Exp(T_Const(0)));
}

Tr_exp Tr_intExp(int val) {
    return Tr_Ex(T_Const(val));
}
// 字符串表达式
Tr_exp Tr_stringExp(string str, Tr_level level) {
    // alloc a new label
    Temp_label lab = F_name(level->frame);
    // build a chain
    // 字符串常量类似于静态变量
    gFrags = F_FragList(F_StringFrag(lab,str),gFrags);
    
    // return a NAME not a label
    return Tr_Ex(T_Name(lab));
}
//参数分别为标记，调用层，定义层，参数列表
Tr_exp Tr_callExp(Temp_label fun, Tr_level called, Tr_level defined, Tr_expList args) 
{
    T_expList paralist; // 参数链表
    T_expList tmp;
    while(args) {
        exps = T_ExpList(unEx(args->head),NULL);
        if(list == NULL) {
            list = exps;
        } else {
            exps = exps->tail;
        }
        args = args->tail;        
    }
    // 记录参数
    list = T_expList(unEx(staticLink(defined,called)),list);
    return Tr_Ex(T_Call(T_Name(fun),list));
}

// 二元操作表达式
Tr_exp Tr_opExp(A_oper op, Tr_exp left, Tr_exp right) 
{
    bool isbin = FALSE;
    bool isrel = FALSE;

    T_binOp binop;
    T_relOp relop;

    switch(op) {
        case A_plusOp:   binop = T_plus; isbin = TRUE; break;
        case A_minusOp:  binop = T_minus; isbin = TRUE; break;
        case A_timesOp:  binop = T_mul; isbin = TRUE; break;
        case A_divideOp: binop = T_div; isbin = TRUE; break;
        case A_eqOp:     relop = T_eq; isrel = TRUE; break;
        case A_neqOp:    relop = T_ne; isrel = TRUE; break;
        case A_ltOp:     relop = T_lt; isrel = TRUE; break;
        case A_leOp:     relop = T_le; isrel = TRUE; break;
        case A_gtOp:     relop = T_gt; isrel = TRUE; break;
        case A_geOp:     relop = T_ge; isrel = TRUE; break;
        default: break; 
    }
    if(isbin) {
        return Tr_Ex(T_Binop(binop,unEx(left),unEx(right)));
    } else if(isrel) {
        // 关系表达式，true返回1？false返回0？
        T_stm stm = T_Cjump(relop,unEx(left),unEx(right),NULL,NULL);    
        patchList trues = PatchList(&stm->u.CJUMP.true,NULL);     
        patchList falses = PatchList(&stm->u.CJUMP.false,NULL);       
        return Tr_Cx(trues,falses,stm);
    }     
    assert(0);
}
Tr_exp Tr_ifWithNoElse(Tr_exp cond, Tr_exp then)
{
    Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    struct Cx condT = unCx(cond);
    doPatch(condT.trues,t);
    doPatch(condT.falses,f); 
    if(then->kind == Tr_nx)
    { // 没有返回值的表达式
        return Tr_Nx(T_Seq(condT.stm,T_Seq(T_Label(t),T_Seq(then->u.stm,T_Label(f)))));
    }
    else if(then->kind == Tr_cx)
    {
        return Tr_Nx(T_Seq(condT.stm,T_Seq(T_Label(t),T_Seq(then->u.cx.stm,T_Label(f)))));
    }
    else // 有返回值的语句
    {
        return Tr_Nx(T_Seq(condT.stm,T_Seq(T_Label(t),T_Seq(unEx(then->u.ex),T_Label(f)))));
    }
}
Tr_exp Tr_ifWithElse(Tr_exp cond, Tr_exp then, Tr_exp elseexp)
{
    Temp_label t = Temp_newlabel(), f = Temp_newlabel(), join = Temp_newlabel();
    Temp_temp r = Temp_newtemp();
    Tr_exp result = NULL;
    T_stm joinJump = T_Jump(T_Name(join), Temp_LabelList(join, NULL));
    struct Cx cond = unCx(test);
    doPatch(cond.trues, t);
    doPatch(cond.falses, f);
    if (elsee->kind == Tr_ex) {
        result = Tr_Ex(T_Eseq(cond.stm, T_Eseq(T_Label(t), 
                    T_Eseq(T_Move(T_Temp(r), unEx(then)),
                    T_Eseq(joinJump, T_Eseq(T_Label(f),
                            T_Eseq(T_Move(T_Temp(r), unEx(elsee)), 
                                T_Eseq(joinJump, 
                                T_Eseq(T_Label(join), T_Temp(r))))))))));
    } else {
        T_stm thenStm;
        if (then->kind == Tr_ex) thenStm = T_Exp(then->u.ex);
        else thenStm = (then->kind == Tr_nx) ? then->u.nx : then->u.cx.stm;

        T_stm elseeStm = (elsee->kind == Tr_nx) ? elsee->u.nx : elsee->u.cx.stm;
        result = Tr_Nx(T_Seq(cond.stm, T_Seq(T_Label(t), T_Seq(thenStm,
                    T_Seq(joinJump, T_Seq(T_Label(f),
                            T_Seq(elseeStm, T_Seq(joinJump, T_Label(join)))))))));
    }
    return result;


}
Tr_exp Tr_ifExp(Tr_exp ifexp, Tr_exp thenexp, Tr_exp elseexp) 
{
    if(elseexp)
        return Tr_ifWithNoElse(ifexp,thenexp);
    else
        return Tr_ifWithElse(ifexp,thenexp,elseexp);
}

Tr_exp Tr_whileExp(Tr_exp t, Tr_exp b, Temp_label ldone) {
    Temp_label ltest = Temp_newlabel();
    Temp_label lbody = Temp_newlabel();


    T_exp test = unEx(t);
    T_stm body = unNx(b); 

    return Tr_Nx(T_Seq(T_Label(ltest),
                    T_Seq(T_Cjump(T_ne,test,T_Const(0),lbody,ldone),
                        T_Seq(T_Label(lbody),
                            T_Seq(body,
                                T_Seq(T_Jump(T_Name(ltest),Temp_LabelList(ltest,NULL)),T_Label(ldone)))))));
}
Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label ldone) {

    Temp_temp  temp = Temp_newtemp(); // 临时寄存器变量
    Temp_label ltest = Temp_newlabel();
    Temp_label lbody = Temp_newlabel();

    T_exp loexp = unEx(lo);
    T_exp hiexp = unEx(hi);
    T_stm bodystm = unNx(body); // produce no value
  
// 处理for循环的时候，先比较上下界，在将测试条件放在最后
    return Tr_Nx(T_Seq(T_Move(T_Temp(temp),loexp),
                    T_Seq(T_Label(ltest),
                        T_Seq(T_Cjump(T_lt,T_Temp(temp),hiexp,lbody,ldone),
                            T_Seq(T_Label(lbody),
                                T_Seq(bodystm,
                                    T_Seq(T_Jump(T_Name(ltest),Temp_LabelList(ltest,NULL)),T_Label(ldone))))))));
}

Tr_exp Tr_recordExp(int n, Tr_expList fields)
{
    T_expList args = T_ExpList(T_Binop(T_mul,T_Const(n),T_Const(wordSize)),NULL);
    // TODO: how to call malloc? malloc: run-time library 
    T_exp call = F_externalCall("malloc",args);

    Temp_temp r = Temp_newtemp();
    int count = n;

    T_stm seq = T_Seq(T_Move(T_Temp(r),call),NULL);
    T_stm head = seq, temp;

    while(fields && n) {
        temp = T_Seq(T_Move(T_Mem(T_Binop(T_plus,T_Temp(r),T_Const((count-n)*wordSize))),
                            unEx(fields->head)),NULL);
        fields = fields->tail;
        n--;
        seq->u.SEQ.right = temp;
        seq = seq->u.SEQ.right;
    }

    return Tr_Ex(T_Eseq(head,T_Temp(r)));
}

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp val) {
    return Tr_Nx(T_Move(unEx(var),unEx(val)));
}

// same with record, return r, the address
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) {
    // first : malloc some memory
    T_expList args0 = T_ExpList(T_Binop(T_mul,unEx(size),T_Const(wordSize)),NULL);
    T_exp call0 = F_externalCall("malloc",args0);
    Temp_temp r= Temp_newtemp();

    // second : init array
    T_expList args1 = T_ExpList(T_Binop(T_mul,unEx(size),T_Const(wordSize)),
                    T_ExpList(unEx(init),NULL));
    T_exp call1 = F_externalCall("initArray",args1);
    T_stm seq = T_Seq(T_Move(T_Temp(r),call0),T_Exp(call1));

    return Tr_Ex(T_Eseq(seq,T_Temp(r)));
}

// should be a Tr_Nx and actually a MOVE T_stm
Tr_exp Tr_varDec(Tr_access acc, Tr_exp init) {
    // var must be in current frame, so use FP directly
    T_exp var = F_Exp(acc->access, T_Temp(F_FP()));
    T_exp val = unEx(init);
    return Tr_Nx(T_Move(var,val));
}

Tr_exp Tr_funDec(Tr_level level, Tr_exp body) {
    T_stm sbody = unNx(body);

    //TODO: add Move body ot RV

    // build a chain
    gFrags = F_FragList(F_ProcFrag(sbody,level->frame),gFrags);

    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_typeDec() {
    return Tr_Ex(T_Const(0));
}

F_fragList Tr_getResult(void) {
    return gFrags;
}
//记住片段
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals)
{
    F_frag pfrag = F_ProcFrag(unNx(body), level->frame);
    fragList = F_FragList(pfrag, fragList);
}
