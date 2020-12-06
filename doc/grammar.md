GoldScorpion Grammar
=====================

```
program -> declaration* EOF

declaration -> \n* (
				annotation |
				typeDecl |
				funDecl |
				varDecl |
				importDecl |
				statement )
				\n*

annotation -> "@" "[" expression ( "," expression )* "]" \n

typeDecl -> "type" IDENTIFIER \n \n*
			(parameter \n*)+
			(funDecl \n*)*
			"end"

funDecl -> "function" IDENTIFIER? "(" parameter ( "," parameter )* ")" ( "as" IDENTIFIER )? declaration* "end"

varDecl -> "def" parameter ( "=" expression )? \n

importDecl -> "import" PATH \n

statement -> exprStatement |
			 forStatement |
			 ifStatement |
			 returnStatement |
			 asmStatement |
			 whileStatement

exprStatement -> expression \n

forStatement -> "for" IDENTIFIER "=" IDENTIFIER "to" IDENTIFIER ( "every" IDENTIFIER )? \n
				declaration* "end"

ifStatement -> "if" expression "then" declaration*
				( "else" "if" expression "then" declaration* )*
				( "else" declaration* )?
				"end"

returnStatement -> "return" expression? \n

asmStatement -> "asm" \n TEXT \n "end"

whileStatement -> "while" expression \n declaration* "end"

parameter -> IDENTIFIER "as" type
arguments	-> expression ( "," expression )*

type -> ( "u8" | "u16" | "u32" | "s8" | "s16" | "s32" |	"string" | IDENTIFIER ) ( "[" NUMBER "]" )?

expression -> assignment
assignment -> ( call "." )? IDENTIFIER "=" assignment
			| logic_or
logic_or   -> logic_xor ( "or" logic_xor )*
logic_xor  -> logic_and ( "xor" logic_and )*
logic_and  -> bw_or ( "and" bw_or )*
bw_or      -> bw_xor ( "|" bw_xor )*
bw_xor     -> bw_and ( "^" bw_and )*
bw_and     -> equality ( "&" equality )*
equality   -> comparison ( ( "!=" | "==" ) comparison )*
comparison -> bitwise ( ( ">" | ">=" | "<" | "<=" ) bitwise )*
bitwise    -> term ( ( ">>" | "<<" ) term )*
term       -> factor ( ( "-" | "+" ) factor )*
factor     -> unary ( ( "/" | "*" | "%" ) unary )*
unary      -> ( "not" | "-" ) unary | call
call       -> primary ( "(" arguments? ")" | "." IDENTIFIER )*
primary    -> "this" | NUMBER | STRING | IDENTIFIER | "(" expression ")"
               | "super" "." IDENTIFIER
```