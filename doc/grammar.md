GoldScorpion Grammar
=====================

```
program -> declaration* EOF

declaration -> typeDecl |
				funDecl |
				varDecl |
				statement;


typeDecl -> "type" IDENTIFIER \n?
			(parameter \n?)*
			(funDecl \n?)*
			"end"

funDecl -> "function" IDENTIFIER "(" parameter ( "," parameter )* ")" \n? declaration* \n? "end"

varDecl -> "def" parameter ( "=" expression )?

statement -> exprStatement |
			 forStatement |
			 ifStatement |
			 returnStatement |
			 whileStatement

exprStatement -> expression \n

forStatement -> "for" IDENTIFIER "=" IDENTIFIER "to" IDENTIFIER ( "every" IDENTIFIER )?
				\n? declaration* \n? "end"

ifStatement -> "if" expression "then" \n? declaration* \n?
				( "else" "if" expression "then" \n? declaration* \n? )*
				( "else" \n? declaration* \n? )?
				"end"

returnStatement -> "return" expression? \n

whileStatement -> "while" expression \n? statement* \n? "end"

parameter -> IDENTIFIER "as" type
arguments	-> expression ( "," expression )*

type -> "u8" |
		"u16" |
		"u32" |
		"s8" |
		"s16" |
		"s32" |
		"string" |
		IDENTIFIER;

expression -> assignment
assignment -> ( call "." )? IDENTIFIER "=" assignment
			| logic_or
logic_or   -> logic_xor ( "or" logic_xor )*
logic_xor  -> logic_and ( "xor" logic_and )*
logic_and  -> equality ( "and" equality )*
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