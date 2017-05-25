%{
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "y.tab.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
    charPos=1;
    return 1;
}

void adjust(void)
{
    EM_tokPos = charPos;
    charPos += yyleng;
}

char* str = NULL;
int len, maxlen;

void str_init() {
    str = (string)checked_malloc(100);
    maxlen = 100;
    str[0] = '\0';
    len = 0;
}

void str_add(char c) {
    if (len + 1 == maxlen) {
        str = (string)realloc(str, maxlen + 100);
        if (!str) {
            printf("malloc error!\n");
            exit(1);
        }
        maxlen += 100;
    }
    str[len++] = c;
    str[len] = '\0';
}

void str_del() {
    free(str);
    str = NULL;
}

//备注：STR部分没有严格遵照tiger的语法来写，日后可以考虑进行修正

%}

%x COMMENT STR

%%

int count = 0;

<COMMENT>{
    "/*"                    	{adjust(); count++;}
    "*/"                    	{adjust(); count--; if (count == 0) BEGIN(0);}
    "\n"                        {adjust(); EM_newline();}
    .                       	adjust();
}

<STR>{
	\"							{adjust(); yylval.sval = String(str); str_del(); BEGIN(0); return STRING;}
    \\n                         {adjust(); str_add('\n');}
    \\t                         {adjust(); str_add('\t');}
    \\\"                        {adjust(); str_add('\"');}
    \\\\                        {adjust(); str_add('\\');}
	.                       	{adjust(); str_add(yytext[0]);}
}

[ \t]*                          adjust(); 
"\n"                            {adjust(); EM_newline();}

"/*"                        	{adjust(); count++; BEGIN(COMMENT);}
\"                          	{adjust(); str_init(); BEGIN(STR);}

array							{adjust(); return ARRAY;}
if								{adjust(); return IF;}
then							{adjust(); return THEN;}
else							{adjust(); return ELSE;}
while							{adjust(); return WHILE;}
for								{adjust(); return FOR;}
to								{adjust(); return TO;}
do								{adjust(); return DO;}
let								{adjust(); return LET;}
in 								{adjust(); return IN;}
end								{adjust(); return END;}
of								{adjust(); return OF;}
break							{adjust(); return BREAK;}
nil								{adjust(); return NIL;}
function						{adjust(); return FUNCTION;}
var								{adjust(); return VAR;}
type							{adjust(); return TYPE;}

[A-Za-z]([A-Za-z0-9]|"_")*		{adjust(); yylval.sval = String(yytext); return ID;}
[0-9]+							{adjust(); yylval.ival = atoi(yytext); return INT;}

","								{adjust(); return COMMA;}
":"								{adjust(); return COLON;}
";"								{adjust(); return SEMICOLON;}
"("								{adjust(); return LPAREN;}
")"								{adjust(); return RPAREN;}
"["								{adjust(); return LBRACK;}
"]"								{adjust(); return RBRACK;}
"{"								{adjust(); return LBRACE;}
"}"								{adjust(); return RBRACE;}
"."								{adjust(); return DOT;}
"+"								{adjust(); return PLUS;}
"-"								{adjust(); return MINUS;}
"/"								{adjust(); return DIVIDE;}
"*"								{adjust(); return TIMES;}
"="								{adjust(); return EQ;}
"<>"							{adjust(); return NEQ;}
">"								{adjust(); return LT;}
">="							{adjust(); return LE;}
"<"								{adjust(); return GT;}
"<="							{adjust(); return GE;}
"&"								{adjust(); return AND;}
"|"								{adjust(); return OR;}
":="							{adjust(); return ASSIGN;}					

.                               {adjust(); EM_error(EM_tokPos, "illegal token");}								