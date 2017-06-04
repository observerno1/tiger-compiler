#include "semant.h"
#include "MyEnv.h"
#include "translate.h"
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
  struct expty e;
  e.exp = exp;
  e.ty = ty;
  return e;
}
typedef struct SymStack_ *SymStack;
// 用来检测一个域内类型名字是否冲突
struct SymStack_{
	S_symbol array[500];
	int pos;
}
SymStack CheckStack = NULL;
SymStack FuncStack = NULL;
static loopNum = 0; // 记录loop的次数
// if add succeed, no collision
bool addItem(SymStack m,S_symbol s)
{
	if(m == NULL)
	{
		return false;
	}
	else
	{
		if(IsTypeCollision(m,s))
		{
			return false;
		}
	}
	m->array[m->pos++] = s;
	return true;
}

// 存在返回true，否则返回false
bool IsTypeCollision(SymStack m,S_symbol s)
{
	if(m == NULL)
		return false;
	int i=0;
	for(; i < m->pos; i++)
	{
		if(m->array[i] == s)
			return true;
	}
	return false;
}
void resetStack(SymStack stack)
{
	if(stack == NULL)
	{
		return;
	}
	stack->pos = 0;
}
SymStack Stack_init()
{
	SymStack s = checked_malloc(sizeof(*s));
	s->pos = 0;
	return s;
}
Ty_ty actual_ty(Ty_ty t)
{
	while (t && t->kind == Ty_name)
		t = t->u.name.ty;
	return t;
}
bool Type_Compare(Ty_ty t1,Ty_ty t2)
{
	assert(t1&&t2);
	Ty_ty a1 = actual_ty(t1);
	Ty_ty a2 = actual_ty(t2);
	if(a1 == a2)
		return true;
	else if(a1->kind == Ty_record && a2->kind == Ty_nil ||a1->kind == Ty_nil && a2->kind == Ty_record )
	{
		return true;
	}
	else
		return false;
}
// 从一个声明的中的 ty转为需要的类型表示，可能是一个record 或者 Ty_name或者 array
/*struct A_namety_ {
    S_symbol name;
    A_ty ty;
};
*/
void SEM_transProg(A_exp exp)
{
	S_table tEnv = E_base_tenv();
	S_table vEnv = E_base_eenv();
	struct expty result = transExp(tEnv,vEnv,Tr_outermost(),NULL,exp);
	Tr_procEntryExit(Tr_outermost(),exp,NULL);
	return Tr_getResult();
}
Ty_ty transTy(S_table tenv, A_ty ty)
{
	if(ty == NULL)
		return NULL;
	switch(ty->kind)
	{
		case A_nameTy:
		{
			Ty_ty temp = S_look(tenv,ty->u.name);
			if(!temp)
			{
				EM_error(ty->pos,"type undefined %s",S_name(ty->u.name));
				return Ty_Int();		
			}
			else
				return Ty_Name(ty->u.name,temp);
			break;
		}
		case A_recordTy:
		// 必须检查域类型名字是否冲突
		{
			A_fieldList record = ty->u.record;
			Ty_fieldList typeList =  NULL;
			A_field node = record->head;
			if(record == NULL)
			{
				return Ty_Record(NULL);
			}
			resetStack(CheckStack);
			while(typeList!=NULL)
			{
				node = typeList->head;
				if(!addItem(CheckStack,node->name))
				{
					EM_error(node->pos,"redelartion of %s",S_name(node->name));
					typeList = typeList->tail;
					continue;
				}
				// 域内名字没有冲突
				Ty_ty t1 = S_look(tenv,node->typ);
				if(!t1)
				{ // 空了
					EM_error(node->pos,"type undefined %s\n",S_name(node->typ));
					t1 = Ty_Int();  //用int类型替代
				}
				Ty_field f1 = Ty_Field(node->name,t1);
				typeList = Ty_FieldList(f1,typeList);
				record = record->tail;
			}
			resetStack(CheckStack);
			return Ty_Record(typeList);
		}
		case A_arrayTy:
		{
			// array 保存着数组元素类型的sym
			Ty_ty t1 = S_look(tenv,ty->u.array);
			if(!t1)
			{
				EM_error(ty->pos,"type undefined %s ", S_name(ty->u.array));
				return Ty_Array(Ty_Int()); // 先用int类型取代
			}
			else
				return Ty_Array(t1);
		}
		default:
			EM_error(ty->pos,"unexpected type %s",S_name(ty->u.name));
			return Ty_Int();
	}
}
//根据参数列表返回函数的参数类型链表
Ty_tyList makeFormalTyList(S_table tenv,A_fieldList a)
{
	if (!a)
		return NULL;
	// 重复
	if (!addItem(CheckStack, a->head->name)) {
		EM_error(a->head->pos, "redeclaration of '%s'", S_name(a->head->name));
		return makeFormalTyList(tenv, a->tail);
	}
	Ty_ty t = S_look(tenv, a->head->typ);
	if (!t) {
		EM_error(a->head->pos, "undefined type in function paramList with '%s'",S_name(a->head->typ));
		t = Ty_Int(); //未找到的类型默认为int，便于继续处理
	}
	return Ty_TyList(t, makeFormalTyList(tenv, a->tail));
}
//根据声明节点扩展环境
/*
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
	    struct {
            S_symbol var;       //变量的ID
            S_symbol typ;       //变量的类型名称
            A_exp init;         //变量的值，由一个exp指定
            bool escape;
        } var;
	    A_nametyList type;
    } u;
};

*/
// 加入树结构之后修改函数
// void transDec(S_table venv, S_table tenv, A_dec d)
Tr_exp transDec(S_table venv, S_table tenv, Tr_level level, Temp_label lbreak, A_dec d)
{
{
	if(!CheckStack)
		CheckStack = Stack_init();
	//这里只对单个声明扩建环境，外层应该是一个for循环调用这个函数，进行一连串声明的扩展
	switch(d->kind)
	{
		case A_typeDec:{
			A_nametyList type = d->u.type;
			A_namety tmp;
			for(;type; type->tail)
			{
				tmp = type->head;
				if(!addItem(CheckStack,tmp->name))
				{
					EM_error(d->pos,"redelartion of %s",S_name(tmp->name));
					continue;
				}
				// 第一轮先把名字放进环境表里
				S_enter(tenv,Ty_Name(tmp->name,NULL));
			}
			resetStack(CheckStack);
			type = d->u.type;
			for(;type; type->tail)
			{
				tmp = type->head;
				if(!addItem(CheckStack,tmp->name))
				{
					continue;
				}
				// 第二轮更新定义
				Ty_ty t1 = S_look(tenv,tmp->name);
				t1->u.name.ty = transTy(tenv,tmp->ty);
			}
			// 检查是不是存在递归定义链
			// 如果存在 D = B B = C C = D 这样的递归环，那么沿着该链寻找必定能找到起点
			for(type = d->u.type;type; type = type->tail)
			{
				tmp = type->head;
				resetStack(CheckStack); // 全清空标记栈
				assert(tmp);
				Ty_ty t1 = S_look(tenv,tmp->name);
				addItem(CheckStack,tmp->name); // 起始符号加入
				t1 = t1->u.name.ty;   // 找到 C = D，D对应ty描述指针
				while(t1 && t1->kind == Ty_name)
				{
					// C = D D = C
					// c的定义链如下 Ty_Namety (C )-> (D，)-> (D,)
					// D定义链如下  Ty_Namety (D )-> (C, )-> C(D, NULL)
					if(!addItem(CheckStack,t1->u.name.sym))
					{
						EM_error(d->pos,"recursive type defintion %s",S_name(t1->u.name.sym));
						// 存在递归要不要重定义为其他类型？
					// 	t1->u.name.ty = Ty_Int();
						break;
					}
					// 根据上述定义方式，每次加一个类型定义的时候实际上会加上两中间节点，需要跳过一个
					t1 = t1->u.name.ty;
					t1 = t1->u.name.ty;
				}
			}
			return Tr_typeDec();
		}
		// 函数定义，需要将参数列表和返回值放入值环境中
		case A_functionDec:
		{

			A_fundecList Flist = d->u.function;
			A_fundec OneFunc = NULL;
			FuncStack = Stack_init();
			// 处理函数的时候先将函数头部放入变量环境表中
			S_symbol Fname;          //函数ID
   	 		A_fieldList Fparams;     //参数
    		S_symbol Fresult;        //返回值类型ID
    		Ty_ty typeResult;
    		Ty_TyList paramList;  //函数参数列表
    	//	makeEscapeFormalList
			while(Flist!= NULL)
			{

				OneFunc = Flist->head;
				assert(OneFunc);
				Fname = OneFunc->name;
				Fparams = OneFunc->params;
				// m每一函数声明的逃逸变量链表
				U_boolList escapeList = makeEscapeFormalList(OneFunc->params);
				Temp_label currentLabel = OneFunc->name;  // 名字作为label标记
				Tr_level currentLevel = Tr_newLevel(level,currentLabel,escapeList);
				if(!addItem(FuncStack,Fname))
				{
					// 重复定义，跳过
					EM_error(d->pos,"redefinition of function: %s",S_name(Fname));
					Flist = Flist->tail;
					continue;
				}
				Fresult = OneFunc->result;
				if(!Fresult)
				{
					typeResult = S_look(tenv,Fresult);
					if(!typeResult)
					{
						EM_error(d->pos,"unknown return type of function: %s, %s",S_name(Fname),S_name(Fresult));
						Flist = Flist->tail;
						continue;
					}
					// 跳过此函数的处理
				}
				else
				{
					// 没有返回类型定义，那么就是void
					typeResult = Ty_Void();
				}
				// 获取参数列表
				resetStack(CheckStack);
				paramList = makeFormalTyList(tenv,Fparams);
				S_enter(venv,OneFunc->name,E_FunEntry(currentLevel,currentLabel,paramList,typeResult));

				Flist = Flist->tail;
			}
			Flist = d->u.function;
			// 第二轮循环，把函数体放入
			resetStack(FuncStack);
			while(Flist != NULL)
			{
				OneFunc = Flist->head;
				// 参数名字放入
				if(!addItem(FuncStack,OneFunc->name))
				{
					Flist = Flist->tail;
					continue;
				}
				Fresult = OneFunc->result;
				if(!Fresult)
				{
					typeResult = S_look(tenv,Fresult);
					if(!typeResult)
					{
						// 未知类型名字
						EM_error(d->pos,"unknown return type of function: %s, %s",S_name(Fname),S_name(Fresult));
						Flist = Flist->tail;
						continue;
					}
					// 跳过此函数的处理
				}
				else
				{
					// 没有返回类型定义，那么就是void
					typeResult = Ty_Void();
				}
				// 找到刚刚放进去的函数头描述
				E_enventy funEntry = S_look(vEnv,OneFunc->name);
				Tr_accessList accList = funEntry->u.func.level->formals;
				S_beginScope(venv);
				Ty_ty tmpTy;
				A_fieldList l;
				resetStack(CheckStack);

				for(l = OneFunc->params; l && accList; l = l->tail,accList = accList->tail)
				{
					if(!addItem(CheckStack,l->head->name))
					{// 重复定义类型，跳过
						continue;
					}
					tmpTy = S_look(tenv,l->head->typ); // 找类型
					if(!tmpTy)
						tmpTy = Ty_Int(); // 重置为int
					S_enter(venv,l->head->name,E_VarEntry(accList->head,tmpTy));
				}
				struct expty e = transExp(venv,tenv,funEntry->u.func.level,lbreak,OneFunc->body);
				if(!Type_Compare(e.ty,typeResult)
				{
					EM_error(OneFunc->pos,"function declartion type not consistent with return type '%s'",S_name(OneFunc->name));
				}
				// 保存函数片段
				Tr_exp res = Tr_funDec(OneFunc->u.func.level,e.exp);
				S_endScope(venv);
				Flist = Flist->tail;
			}
			return Tr_Ex(T_Const(0));
		}
		// 变量声明
		case A_varDec:
		{

			struct expty e =transExp(venv,tenv,level,lbreak,d->u.var.init);
			if(d->u.var.typ) // 类型非空
			{
				Ty_ty t1 = S_look(tenv,d->u.var.typ);
				if(!t1)
				{
					EM_error(d->pos,"undefined type %s",S_name(d->u.var.typ));
				}
				else
				{
					// 类型比较
					if(!Type_Compare(t1,e.ty))
						EM_error(d->pos,"var init with mismatch type");
					//对于var的声明，需要分配acces
					Tr_access acc = Tr_allocLocal(level,true); // 逃逸
					S_enter(venv,d->u.var.var,E_VarEntry(t1));
					return Tr_varDec(acc,e.exp);
				}
			}
			// 类型是空的不能接受nil的初值初始化
			else if(e.ty->kind == Ty_nil && !d->u.var.typ)
			{
				EM_error(d->pos,"initialize with wrong type for var %s",S_name(d->u.var.var));
			}
			else if(e.ty->kind == Ty_void)
			{
				EM_error(d->pos,"initialize with no type for var %s",S_name(d->u.var.var));
			}
			// 最后还是分配本地单元，tiger假设所有变量的长度都一个机器字
			//对于var的声明，需要分配acces
			Tr_access acc = Tr_allocLocal(level,true); // 逃逸
			S_enter(venv,d->u.var.var,E_VarEntry(t1));
			return Tr_varDec(acc,e.exp);

		}
		default:
			break;
	}
}
// 计算表达式得到返回值
struct expty transExp(S_table venv, S_table tenv,Tr_level level,Temp_label lbreak ,A_exp a)
{
	switch(a->kind)
	{
		case A_varExp:
			return transVar(venv,tenv,level,lbreak,a->u.var);	 
		case A_nilExp:
			return expTy(Tr_nilExp(),Ty_Nil());
		case A_intExp:
			return expTy(Tr_intExp(a->u.intt),Ty_Int()); // 返回int 类型指针
		case A_stringExp:
			return expTy(Tr_stringExp(a->u.stringg),Ty_String());
		case A_callExp:
			{ // 调用函数的
				E_enventy fun = S_look(venv,a->u.call.func);
				if(!fun)  // 函数描述结构指针为null
				{
					EM_error(a->pos,"undeclared funtion name %s",S_name(a->u.call.func));
					return expTy(NULL,Ty_Void());
				}
				else if(fun->kind == E_varEntry)
				{
					EM_error(a->pos,"%s is a variable not a function type",S_name(a->u.call.func));
					return expTy(NULL,fun->u.var.ty);
				}
				Ty_TyList fomals = fun->func.formals;
				Ty_ty result = fun->func.result;
				A_expList arg = fun->u.call.args;
				Tr_ExpList callParam = NULL;
				// 比较参数类型是不是一样的
				for(; arg && fomals; arg = arg->tail, fomals = fomals->tail)
				{
					struct expty tmp = transExp(venv,tenv,level,lbreak,arg->head);
					if(!Type_Compare(fomals->head,tmp,ty))
					{
						EM_error(a->pos,"parameter type mismatch");
					}
					// 无论如何还是把参数放到节点上去
					if(!callParam)
						callParam = Tr_ExpList(tmp.exp,callParam);
					else
					{  

						Tr_ExpList_append(callParam,tmp.exp);
					}
					// 这里参数链表是顺序的
				}
				if(fomals)
				{
					// 还有参数
					EM_error(a->pos,"parameter too few");
				}
				if(arg)
				{
					EM_error(a->pos,"parameter too many");
				}
				Tr_exp cExp = Tr_callExp(fun->u.func.label,level,fun->u.func.level,callParam);
				return expTy(cExp,result);
			}
		case A_opExp:
			{
				struct expty leftE = transExp(venv,tenv,level,lbreak,a->u.op.left);
				struct expty rigthE = transExp(venv,tenv,level,lbreak,a->u.op.rigth);
				switch(a->u.op.oper)
				{
					case A_plusOp:
					case A_minusOp:
					case A_timesOp:
					case A_divideOp:
						{
							assert(leftE.ty&&rigthE.ty);
							if(leftE.ty->kind != Ty_int || rigthE.ty->kind!= Ty_int)
							{
								EM_error(a->pos,"integer required for math expression");
							}
							Tr_exp exp = Tr_opExp(a->u.op.oper,leftE,rigthE);
							return expTy(exp,Ty_Int());
						}
					case A_eqOp:
					case A_neqOp:
						{  // 比较运算
						   // 不能为空，运算,除此之外都能比较
							if(leftE.ty>kind == Ty_void || rigthE.ty->kind == Ty_void)
							{
								EM_error(a->u.op.left->pos,"expression has no value");
							}
							if(!Type_Compare(leftE.ty,rigthE.ty))
							{
								//类型不一致
								EM_error(a->u.left->pos,"the two sides of compared expression have no the same types");
							}
							// 比较结果用int表示
							Tr_exp exp = Tr_opExp(a->u.op.oper,leftE,rigthE);
							return expTy(exp,Ty_Int());
						}
					case A_ltOp:
    				case A_leOp:
    				case A_gtOp:
    				case A_geOp:
    				{ // 不等式比较
    				  // 两边必须为int类型或者string类型，同时
    					if(!Type_Compare(leftE.ty,rigthE.ty)||leftE.ty->kind!= Ty_int||rigthE.ty->kind!=Ty_string)
    					{
    						EM_error(a->u.op.left->pos,"required int type or string at the same time");
    					}
    					// 比较结果问保存为int类型
						Tr_exp exp = Tr_opExp(a->u.op.oper,leftE,rigthE);
						return expTy(exp,Ty_Int());
    				}
				}
			}
		case A_recordExp:
			{ // 记录类型
				S_symbol tmpSym = a->u.record.typ;
				Ty_ty ty1 = actual_ty(S_look(tenv,tmpSym));
				// 比较两者是不是一样的
				if(!ty1)
				{
					EM_error(a->pos,"no such type %s",S_name(tmpSym));
					return expTy(NULL,Ty_Record(NULL));
				}
				else if(ty1->kind != Ty_record)
				{
					EM_error(a->pos,"%s is not a record type",S_name(tmpSym));
			//		struct expty e = transVar(venv,tenv,level,lbreak,)
					return expTy(NULL,ty1);
				}
				// 比较参数
				Ty_FieldList fomals = ty1->u.record;
				A_efieldList arg = a->u.record.fields;
				Tr_expList paramList = NULL;
				int cuunt = 0;
				for(; arg && fomals; arg = arg->tail, fomals = fomals->tail)
				{
					
					if(fomals->head->name != arg->head->name)
					{
						EM_error(a->pos,"member of '%s' is '%s' not %s",S_name(tmpSym),S_name(fomals->head->name),S_namearg->head->name));
						continue; // 下面不用比较了
					}
					struct expty tmp = transExp(venv,tenv,level,lbreak,arg->head->exp);
					if(!Type_Compare(fomals->head->ty,tmp.ty))
					{ // 原定类型和计算赋值类型不一致
						EM_error(a->pos,"parameter type mismatch for member %s",S_name(fomals->head->name));
					}
					count ++;
					if(!paramList)
					{
						paramList = Tr_ExpList(tmp,paramList);
					}
					else
					{
						Tr_ExpList_append(paramList,tmp);
					}

				}
				if(fomals)
				{
					// 还有参数
					EM_error(a->pos,"parameter too few");
				}
				if(arg)
				{
					EM_error(a->pos,"parameter too many");
				}
				return expTy(Tr_recordExp(count,paramList),ty1);
			}
		case A_seqExp:
			// 
			{
				A_expList tmplist = a->u.seq;
				if(!tmplist)
				{ // empty ?
					return expTy(NULL,Ty_Void());
				}
				struct expty result = NULL;
				Tr_expList seqList = NULL;
				for(;tmplist;tmplist = tmplist->tail)
				{
					result  = transExp(venv,tenv,level,lbreak,tmplist->head);
					if(!seqList)
					{
						seqList = Tr_ExpList(seqList,result.exp);
					}
				}
				// 最后一=个作为结果
				// result  = transExp(venv,tenv,level,lbreak,tmplist->head);
				return expTy(Tr_seqExp(seqList),result.ty);				
			}
		case A_assignExp:
			{ // 赋值表达式
				struct expty leftV = transVar(venv,tenv,level,lbreak,a->u.assign.var);
				struct expty rightV = transExp(venv,tenv,level,lbreak,a->u.assign.exp);
				if(!Type_Compare(leftV.ty,rightV.ty))
				{
					EM_error(a->pos,"assign type mismatch");
				}
				//赋值运算没有返回值
				retrun expTy(Tr_assignExp(leftV,rightV),Ty_Void());
			}
		case A_ifExp:
			{ // 对于if else语句还是检查条件和body部分
				// 计算条件部分
				struct expty cond = transExp(venv,tenv,level,lbreak,a->u.iff.test);
				if(cond.ty->kind != Ty_int)
				{// 不是int返回类型必定错了
					EM_error(a->pos,"the condition exp for if has no bool result");
				}
				struct expty part1 = transExp(venv,tenv,level,lbreak,a->u.iff.then);
				if(a->u.iff.elsee)
				{
					// if else必须有相同的返回值
					struct expty part2 = transExp(venv,tenv,level,lbreak,a->u.iff.elsee);
					if(!Type_Compare(part1.ty,part2.ty))
						EM_error(a->pos,"the then and else part of if if have no same types");
					// if(part2.ty->kind != Ty_void)
					// {
					// 	EM_error(a->pos,"the else part of if else is not void type");
					// }
					return expTy(Tr_ifExp(cond,part1,part2),part1.ty);
				} 
				// if else 应该是可以有返回值的 print(if a = 1 then "0" else "1");
				// if(part1.ty->kind != Ty_void)
				// {
				// 	EM_error(a->pos,"the then part of if else is not void type");
				// }
				// 在做语义分析之前先设置为void
				return expTy(Tr_ifExp(cond,part1,NULL),part1.ty);
			}
		 case A_whileExp:
		 	{
		 		struct expty cond = transExp(venv,tenv,level,lbreak,a->u.whilee.test);
		 		loopNum ++;
		 		struct expty body = transExp(venv,tenv,level,lbreak,a->u.whilee.body);
		 		loopNum --;
		 		if(cond.ty->kind != Ty_int)
		 		{
		 			EM_error(a->pos,"the condtion type of while loop is not bool type");
		 		}
		 		if(body.ty->kind != Ty_Void)
		 		{
		 			EM_error(a->pos,"the body of while loop is not void type");
		 		}

		 		return expTy(Tr_whileExp(cond,body),Ty_Void());
		 	}
		 case A_forExp:
		 	{
		 		// for 循环转化为while循环
		 		// struct expty lowBound = transExp(venv,tenv,a->u.forr.lo);
		 		// struct expty upBound = transExp(venv,tenv,a->u.forr.hi);
		 		// loopNum ++ ;
		 		// S_beginScope(venv)
		 		// S_enter(venv,a->u.forr.var,E_varEntry(Ty_Int()));
		 		// struct expty body = transExp(venv,tenv,a->u.forr.body);
		 		// S_endScope(venv);
		 		// loopNum --;
		 		// if(lowBound.ty->kind != Ty_int || upBound.ty->kind != Ty_int)
		 		// {
		 		// 	EM_error(a->pos,"the bouds of for loop is not integer type");
		 		// }
		 		// // 
		 		// if(body.ty->kind != Ty_void)
		 		// {
		 		// 	EM_error(a->pos,"the body of for loop is not void type");
		 		// }
		 		// return expTy(NULL,Ty_Void());
		 					/* Convert a for loop into a let expression with a while loop */
				A_dec i = A_VarDec(a->pos, a->u.forr.var, NULL, a->u.forr.lo);
				A_dec limit = A_VarDec(a->pos, S_Symbol("limit"), NULL, a->u.forr.hi);
				A_dec test = A_VarDec(a->pos, S_Symbol("test"), NULL, A_IntExp(a->pos, 1));
				A_exp testExp = A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol("test")));
				A_exp iExp = A_VarExp(a->pos, A_SimpleVar(a->pos, a->u.forr.var));
				A_exp limitExp = A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol("limit")));
				A_exp increment = A_AssignExp(a->pos, 
					A_SimpleVar(a->pos, a->u.forr.var),
					A_OpExp(a->pos, A_plusOp, iExp, A_IntExp(a->pos, 1)));
				A_exp setFalse = A_AssignExp(a->pos, 
					A_SimpleVar(a->pos, S_Symbol("test")), A_IntExp(a->pos, 0));
				/* The let expression we pass to this function */
				A_exp letExp = A_LetExp(a->pos, 
					A_DecList(i, A_DecList(limit, A_DecList(test, NULL))),
					A_SeqExp(a->pos,
						A_ExpList(
							A_IfExp(a->pos,
								A_OpExp(a->pos, A_leOp, a->u.forr.lo, a->u.forr.hi),
								A_WhileExp(a->pos, testExp,
									A_SeqExp(a->pos, 
										A_ExpList(a->u.forr.body,
											A_ExpList(
												A_IfExp(a->pos, 
													A_OpExp(a->pos, A_ltOp, iExp, 
														limitExp),
													increment, setFalse), 
												NULL)))), 
								NULL),
							NULL)));
				struct expty e = transExp(venv, tenv, breakk,level,letExp);
				return e;
		 	}
		 case A_breakExp:
		 	{
		 		if(loopNum == 0)
		 		{
		 			EM_error(a->pos,"no loop to break");
		 		}
		 		Tr_exp break1 = Tr_breakExp(lbreak);
		 		return expTy(break1,Ty_Void());
		 	}
		 case A_letExp:
		 	{ // 声明语句
		 	// 当作一串sequence执行
		 		S_beginScope(tenv);
		 		S_beginScope(venv);
		 		struct expty result;
		 		Tr_expList sequenceList = NULL;
		 		A_decList tmplist = a->u.let.decs;
		 		for(;tmplist;tmplist = tmplist->tai)
		 		{
		 			result = transDec(venv,tenv,level,lbreak,tmplist->head);
		 			if(!sequenceList)
		 			{
		 				sequenceList = Tr_ExpList(result.exp,sequenceList);
		 			}
		 			else
		 			{
		 				Tr_ExpList_append(sequenceList,result.exp);
		 			}

		 		}
		 		struct expty bodyRes = transExp(venv,tenv,level,lbreak,a->u.let.body);
		 		if(!sequenceList)
		 		{
		 			sequenceList = Tr_ExpList(bodyRes.exp,sequenceList);
		 		}
		 		else
		 		{
		 			Tr_ExpList_append(sequenceList,bodyRes.exp);
		 		}
		 		S_endScope(tenv);
		 		S_endScope(venv);

		 		return expty(Tr_seqExp(sequenceList,bodyRes.ty));
		 	}
		 case A_arrayExp:
		 	{
		 		Ty_ty ty1 = actual_ty(S_look(tenv,a->u.array.typ));
		 		if(!ty1)
		 		{
		 			EM_error(a->pos,"undeclared type %s",S_name(a->u.array.typ));
		 			return expTy(NULL,Ty_Int());
		 		}
		 		else if(ty1->kind != Ty_array)
		 		{
		 			EM_error(a->pos,"%s is not an array type",S_name(a->u.array.typ));
		 			return expTy(NULL,ty1);
		 		}
		 		struct expty sizeR = transExp(venv,tenv,level,lbreak,a->u.array.size);
		 		struct expty sizeI = transExp(venv,tenv,level,lbreak,a->u.array.init);
		 		if(sizeR.ty->kind != Ty_int)
		 		{
		 			EM_error(a->pos,"array size is not integer");
		 		}
		 		if(!Type_Compare(sizeI.ty,ty->u.array))
		 		{
		 			EM_error(a->pos,"the init type for array %s is not consistent with declartion type",S_name(a->u.array.typ));		 			
		 		}
		 		return expTy(Tr_arrayExp(sizeR.exp,sizeI.exp),ty1);
		 	}
	}

}
// 
struct expty transVar(S_table venv, S_table tenv, Tr_level level,Temp_label lbreak,A_var v)
{
	// 根据Var的表达式返回左值的类型描述
	switch(v->knid)
	{
		case A_simpleVar:
			{
				E_VarEntry e1 = S_look(venv,v->u.simple);
				if(e1 && e1->kind == E_varEntry)
				{
					Tr_exp simVar = Tr_simpleVar(e1->u.var.access,level);
					return expTy(simVar,actual_ty(e1->u.var.ty));
				}
				else
				{
					EM_error(v->pos,"undefined variable or unexpected  variable %s",S_name(v->u.simple));
					return expTy(NULL,Ty_Int());  // 返回int类型
				}
			}
			break;
		case A_fieldVar:
			{ // 根据var.name 找到
				struct expty e = transVar(venv,tenv,level,lbreak,v->u.field.var);
				// 找到点前面的变量描述
				Ty_ty tmpTy = e.ty;
				int index = 0;
				if(tmpTy.kind != Ty_record)
				{
					EM_error(v->pos,"var is not record type, no member");
					return expTy(NULL,Ty_Int());
				}
				else
				{
					Ty_fieldList tmpField = tmpTy.u.record;
					Ty_field node;
					assert(tmpField)
					for( ; tmpField ; tmpField = tmpField->tail)
					{
						node = tmpField->head;
						if(node->name == v->u.field.sym)
						{// record 类型有元素跟sym符号
							Tr_exp e2 = Tr_fieldVar(e.epx,index);
							return expTy(e2,actual_ty(node->ty));
						}
						index++; //下标
					}
					// 找不到该成员
					EM_error(v->pos,"record type has no member %s",S_name(v->u.field.sym));
					return expTy(NULL,Ty_Int());
				}
			}
		case A_subscriptVar:
			{
				struct expty part1 = transVar(venv,tenv,level,lbreak,v->u.subscript.var);
				struct expty part2 = transExp(venv,tenv,level,lbreak,v->u.subscript.exp);
				if(!part1.exp)
				{// 数组名字不存在
					return expTy(NULL,Ty_Void);
				}
				if(part1.ty->kind != Ty_array)
				{
					EM_error(V->pos,"variable not array");
					return expTy(NULL,Ty_Int());
				}
				if(part2.ty->kind != Ty_int)
				{// 下标不是int类型
					EM_error(v->pos,"index is not int type");
					return expTy(NULL,Ty_Int());
				}
				Tr_exp e = Tr_subscriptVar(part1.exp,part2.epx);
				return expTy(e,actual_ty(part1.ty->u.array));
			}
	}
}
// 假设所有参数都是放在栈里面的，逃逸的
U_boolList makeEscapeFormalList(A_fieldList p)
{
	if(p == NULL)
	{
		return NULL;
	}
	U_boolList tmp = NULL;
//	U_boolList t1,t2;
	while(p)
	{
		if(!tmp)
			tmp = U_BoolList(true,NULL);
		tmp = U_BoolList(true,tmp);
		p = p->tai;
	}
	return tmp;
}