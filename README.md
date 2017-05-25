# tiger compiler

词法分析->文法编写->生成抽象语法树->从抽象语法树生成IR树

抽象语法树的工作已经基本完成

但是引用模块中可能有不需要的东西，因为官方提供的模块可以最终进行代码生成，而我们只需要做到IR树。因此日后还可以做一些删繁就简的工作

主要资料来源：
1.http://www.cs.princeton.edu/~appel/modern/c/project.html
提供了官方模块，简化了工作
内容在/reference/frombook/下

2.http://blog.csdn.net/lhfl911/article/details/58170270
这位认真的同学提供了思路，而且经考证是对的
内容在/reference/fromweb/下

（遗憾的是他做到chap6就不做了，可能是因为chap6涉及到底层架构）

3.教材
教材上提供了各模块的介绍，简介了lex、yacc的语法
（真的是简介，但我回头看发现居然都提到过）
