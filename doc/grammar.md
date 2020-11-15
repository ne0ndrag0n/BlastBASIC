GoldScorpion Grammar
=====================

```
program -> declaration* EOF

declaration -> typeDecl |
				funDecl |
				varDecl |
				statement;


typeDecl -> "type" IDENTIFIER
			varDecl*
			funDecl*
			"end"

funDecl -> "function" function "end"

function	-> IDENTIFIER "(" parameters? ")" \n declaration* \n
parameters	-> IDENTIFIER "as" type ( "," IDENTIFIER "as" type )*
arguments	-> expression ( "," expression )*

type -> "u8" |
		"u16" |
		"u32" |
		"s8" |
		"s16" |
		"s32" |
		"string" |
		IDENTIFIER;
```