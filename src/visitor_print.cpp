#include "visitor_print.hpp"
#include "variant_visitor.hpp"
#include <rang.hpp>
#include <iostream>

namespace GoldScorpion {

	// Forward declarations
	static void visit( const Expression& node, int indent );
	static void visit( const Primary& node, int indent );
	static void visit( const Declaration& node, int indent );
	// End forward declarations

	static std::string indentText( int indent, const std::string& str ) {
		std::string result;

		for( int i = 0; i != indent; i++ ) {
			result += "   ";
		}

		result += str;

		return result;
	}

	static void visit( const AssignmentExpression& node, int indent ) {
		std::cout << indentText( indent, "AssignmentExpression" ) << std::endl;
		std::cout << indentText( indent, "<lhs>" ) << std::endl;
		visit( *node.identifier, indent + 1 );

		std::cout << indentText( indent, "<rhs>" ) << std::endl;
		visit( *node.expression, indent + 1 );
	}

	static void visit( const BinaryExpression& node, int indent ) {
		std::cout << indentText( indent, "BinaryExpression" ) << std::endl;
		std::cout << indentText( indent, "<lhs>" ) << std::endl;
		visit( *node.lhsValue, indent + 1 );

		std::cout << indentText( indent, "<operator>" ) << std::endl;
		visit( *node.op, indent + 1 );

		std::cout << indentText( indent, "<rhs>" ) << std::endl;
		visit( *node.rhsValue, indent + 1 );
	}

	static void visit( const UnaryExpression& node, int indent ) {
		std::cout << indentText( indent, "UnaryExpression" ) << std::endl;
		std::cout << indentText( indent, "<operator>" ) << std::endl;
		visit( *node.op, indent + 1 );

		std::cout << indentText( indent, "<operand>" ) << std::endl;
		visit( *node.value, indent + 1 );
	}

	static void visit( const CallExpression& node, int indent ) {
		std::cout << indentText( indent, "CallExpression" ) << std::endl;
		std::cout << indentText( indent, "<operand>" ) << std::endl;
		visit( *node.identifier, indent + 1 );

		std::cout << indentText( indent, "<arguments>" ) << std::endl;
		for( const std::unique_ptr< Expression >& argument : node.arguments ) {
			visit( *argument, indent + 1 );
		}
	}

	static void visit( const Primary& node, int indent ) {
		std::cout << indentText( indent, "Primary" ) << std::endl;

		std::visit( overloaded {
			[ indent ]( const Token& token ) {
				std::cout << indentText( indent, "Type: " ) << token.toString() << std::endl;
				if( token.value ) {
					std::visit( overloaded {
						[ indent ]( long numeric ) {
							std::cout << indentText( indent, "Value: " + std::to_string( numeric ) ) << std::endl;
						},
						[ indent ]( const std::string& string ) {
							std::cout << indentText( indent, "Value: " + string ) << std::endl;
						}
					}, *( token.value ) );
				}
			},
			[ indent ]( const std::unique_ptr< Expression >& expression ) { visit( *expression, indent + 1 ); }
		}, node.value );
	}

	static void visit( const Expression& node, int indent ) {
		std::cout << indentText( indent, "Expression" ) << std::endl;

		std::visit( overloaded {
			[ indent ]( const std::unique_ptr< AssignmentExpression >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< BinaryExpression >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< UnaryExpression >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< CallExpression >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< Primary >& expression ) { visit( *expression, indent ); }
		}, node.value );
	}

	static void visit( const ForStatement& node, int indent ) {
		std::cout << indentText( indent, "ForStatement" ) << std::endl;

		std::cout << indentText( indent, "<index>" ) << std::endl;
		std::cout << indentText( indent + 1, node.index.toString() ) << std::endl;

		std::cout << indentText( indent, "<from>" ) << std::endl;
		std::cout << indentText( indent + 1, node.from.toString() ) << std::endl;

		std::cout << indentText( indent, "<to>" ) << std::endl;
		std::cout << indentText( indent + 1, node.to.toString() ) << std::endl;

		std::cout << indentText( indent, "<every>" ) << std::endl;
		if( node.every ) {
			std::cout << indentText( indent + 1, node.every->toString() ) << std::endl;
		} else {
			std::cout << indentText( indent + 1, "null" ) << std::endl;
		}

		std::cout << indentText( indent, "<body>" ) << std::endl;
		for( const auto& declaration : node.body ) {
			visit( *declaration, indent + 1 );
		}
	}

	static void visit( const IfStatement& node, int indent ) {
		std::cout << indentText( indent, "IfStatement" ) << std::endl;

		std::cout << indentText( indent, "<conditions>" ) << std::endl;
		for( const auto& condition : node.conditions ) {
			visit( *condition, indent + 1 );
		}

		std::cout << indentText( indent, "<bodies>" ) << std::endl;
		int i = 0;
		for( const auto& body : node.bodies ) {
			std::cout << indentText( indent + 1, "<body " + std::to_string( i ) + ">" ) << std::endl;
			
			for( const auto& declaration : body ) {
				visit( *declaration, indent + 2 );
			}

			i++;
		}
	}

	static void visit( const ReturnStatement& node, int indent ) {
		std::cout << indentText( indent, "ReturnStatement" ) << std::endl;

		if( node.expression ) {
			std::cout << indentText( indent, "<expression>" ) << std::endl;
			visit( **node.expression, indent + 1 );
		}
	}

	static void visit( const AsmStatement& node, int indent ) {
		std::cout << indentText( indent, "AsmStatement" ) << std::endl;
		std::cout << indentText( indent, "<length>" ) << std::endl;
		std::cout << std::to_string( node.body.length() ) << std::endl;
	}

	static void visit( const WhileStatement& node, int indent ) {
		std::cout << indentText( indent, "WhileStatement" ) << std::endl;

		std::cout << indentText( indent, "<condition>" ) << std::endl;
		visit( *node.condition, indent + 1 );

		std::cout << indentText( indent, "<body>" ) << std::endl;
		for( const auto& declaration : node.body ) {
			visit( *declaration, indent + 1 );
		}
	}

	static void visit( const ExpressionStatement& node, int indent ) {
		std::cout << indentText( indent, "ExpressionStatement" ) << std::endl;

		visit( *( node.value ), indent + 1 );
	}

	static void visit( const Statement& node, int indent ) {
		std::cout << indentText( indent, "Statement" ) << std::endl;

		std::visit( overloaded {
			[ indent ]( const std::unique_ptr< ExpressionStatement >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< ForStatement >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< IfStatement >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< ReturnStatement >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< AsmStatement >& expression ) { visit( *expression, indent ); },
			[ indent ]( const std::unique_ptr< WhileStatement >& expression ) { visit( *expression, indent ); }
		}, node.value );
	}

	static void visit( const VarDeclaration& node, int indent ) {
		std::cout << indentText( indent, "VarDeclaration" ) << std::endl;

		std::cout << indentText( indent, "<name>" ) << std::endl;
		std::cout << indentText( indent + 1, node.variable.name.toString() ) << std::endl;

		std::cout << indentText( indent, "<type>" ) << std::endl;
		std::cout << indentText( indent + 1, node.variable.type.toString() ) << std::endl;

		std::cout << indentText( indent, "<value>" ) << std::endl;
		if( node.value ) {
			visit( **node.value, indent + 1 );
		} else {
			std::cout << indentText( indent + 1, "null" ) << std::endl;
		}
	}

	static void visit( const FunctionDeclaration& node, int indent ) {
		std::cout << indentText( indent, "FunctionDeclaration" ) << std::endl;

		std::cout << indentText( indent, "<name>" ) << std::endl;
		if( node.name ) {
			std::cout << indentText( indent + 1, node.name->toString() ) << std::endl;
		} else {
			std::cout << indentText( indent + 1, "null" ) << std::endl;
		}

		std::cout << indentText( indent, "<arguments>" ) << std::endl;
		for( const auto& parameter : node.arguments ) {
			std::cout << indentText( indent + 1, "<name>" ) << std::endl;
			std::cout << indentText( indent + 2, parameter.name.toString() ) << std::endl;
			std::cout << indentText( indent + 1, "<type>" ) << std::endl;
			std::cout << indentText( indent + 2, parameter.type.toString() ) << std::endl;
			std::cout << std::endl;
		}

		std::cout << indentText( indent, "<return-type>" ) << std::endl;
		if( node.returnType ) {
			std::cout << indentText( indent + 1, node.returnType->toString() ) << std::endl;
		} else {
			std::cout << indentText( indent + 1, "null" ) << std::endl;
		}

		std::cout << indentText( indent, "<body>" ) << std::endl;
		for( const auto& declaration : node.body ) {
			visit( *declaration, indent + 1 );
		}
	}

	static void visit( const TypeDeclaration& node, int indent ) {
		std::cout << indentText( indent, "TypeDeclaration" ) << std::endl;

		std::cout << indentText( indent, "<name>" ) << std::endl;
		std::cout << indentText( indent + 1, node.name.toString() ) << std::endl;

		std::cout << indentText( indent, "<fields>" ) << std::endl;
		for( const auto& parameter : node.fields ) {
			std::cout << indentText( indent + 1, "<name>" ) << std::endl;
			std::cout << indentText( indent + 2, parameter.name.toString() ) << std::endl;
			std::cout << indentText( indent + 1, "<type>" ) << std::endl;
			std::cout << indentText( indent + 2, parameter.type.toString() ) << std::endl;
			std::cout << std::endl;
		}

		std::cout << indentText( indent, "<functions>" ) << std::endl;
		for( const auto& functionDeclaration : node.functions ) {
			visit( *functionDeclaration, indent + 1 );
		}
	}

	static void visit( const ImportDeclaration& node, int indent ) {
		std::cout << indentText( indent, "ImportDeclaration" ) << std::endl;

		std::cout << indentText( indent, "<path>" ) << std::endl;
		std::cout << indentText( indent + 1, node.path ) << std::endl;
	}

	static void visit( const Annotation& node, int indent ) {
		std::cout << indentText( indent, "Annotation" ) << std::endl;

		std::cout << indentText( indent, "<expressions>" ) << std::endl;
		for( const auto& expression : node.directives ) {
			visit( *expression, indent + 1 );
		}
	}

	static void visit( const Declaration& node, int indent ) {
		std::cout << indentText( indent, "Declaration" ) << std::endl;

		std::visit( overloaded {
			[ indent ]( const std::unique_ptr< Annotation >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< VarDeclaration >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< FunctionDeclaration >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< TypeDeclaration >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< ImportDeclaration >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< Statement >& expression ) { visit( *expression, indent + 1 ); }
		}, node.value );
	};

	static void visit( const Program& node, int indent ) {
		std::cout << indentText( indent, "Program" ) << std::endl;

		for( const auto& statement : node.statements ) {
			visit( *statement, indent + 1 );
		}
	}

	void printAst( const Program& program ) {
		visit( program, 0 );
	}

}