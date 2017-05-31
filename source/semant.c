#include "semant.h"
#include "MyEnv.h"

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
// 从一个声明的中的 ty转为需要的类型表示，可能是一个record 或者 Ty_name或者 array
/*struct A_namety_ {
    S_symbol name;
    A_ty ty;
};
*/
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
				if(!addItem(node->name))
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
void transDec(S_table venv, S_table tenv, A_dec d)
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
				if(!addItem(tmp->name))
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
				if(!addItem(tmp->name))
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
				addItem(tmp->name); // 起始符号加入
				t1 = t1->u.name.ty;   // 找到 C = D，D对应ty描述指针
				while(t1 && t1->kind == Ty_name)
				{
					// C = D D = C
					// c的定义链如下 Ty_Namety (C )-> (D，)-> (D,)
					// D定义链如下  Ty_Namety (D )-> (C, )-> C(D, NULL)
					if(!addItem(t1->u.name.sym))
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
			break;
		}
		// 函数定义，需要将参数列表和返回值放入值环境中
		case A_functionDec:
		{
			A_fundecList Flist = d->u.function;
			A_fundec OneFunc = NULL;
			FuncStack = Stack_init();
			// 处理函数的时候先
			
		}
	}
}
