compiler: errormsg.o util.o lex.yy.o y.tab.o absyn.o symbol.o table.o semant.o MyEnv.o types.o MyFrame.o temp.o tree.o translate.o printtree.o
	gcc -o compiler errormsg.o util.o lex.yy.o y.tab.o absyn.o symbol.o table.o semant.o MyEnv.o types.o MyFrame.o temp.o tree.o translate.o printtree.o

errormsg.o: errormsg.c errormsg.h util.h
	gcc -c errormsg.c

util.o: util.c util.h
	gcc -c util.c

absyn.o: absyn.c util.h symbol.h absyn.h
	gcc -c absyn.cfele

lex.yy.c: lex.l
	flex lex.l

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h absyn.h
	gcc -c lex.yy.c

y.tab.o: y.tab.c
	gcc -c y.tab.c

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

y.tab.c: yacc.y
	yacc -dv yacc.y

symbol.o: symbol.c util.h table.h symbol.h
	gcc -c symbol.c

table.o: table.c util.h table.h
	gcc -c table.c

semant.o: semant.c semant.h MyEnv.h translate.h symbol.h absyn.h errormsg.h types.h
	gcc -c semant.c

MyEnv.o: MyEnv.c MyEnv.h symbol.h types.h util.h
	gcc -c MyEnv.c

types.o: types.c util.h symbol.h types.h
	gcc -c types.c

MyFrame.o: frame.h temp.h util.h
	gcc -c MyFrame.c

temp.o: util.h symbol.h temp.h table.h
	gcc -c temp.c

tree.o: tree.h symbol.h temp.h util.h
	gcc -c tree.c

translate.o: translate.h frame.h tree.h absyn.h
	gcc -c translate.c

printtree.o: util.h symbol.h temp.h tree.h printtree.h
	gcc -c printtree.c

clean:
	rm -f *.o lex.yy.c y.output
