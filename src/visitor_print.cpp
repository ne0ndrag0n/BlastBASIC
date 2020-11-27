#include "visitor_print.hpp"
#include "variant_visitor.hpp"
#include <rang.hpp>
#include <iostream>

namespace GoldScorpion {

	// Forward declarations
	static void visit( const Expression& node, int indent );
	static void visit( const Primary& node, int indent );
	// End forward declarations

	static std::string indentText( int indent, const std::string& str ) {
		std::string result;

		for( int i = 0; i != indent; i++ ) {
			result += "\t";
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
			[ indent ]( const std::unique_ptr< AssignmentExpression >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< BinaryExpression >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< UnaryExpression >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< CallExpression >& expression ) { visit( *expression, indent + 1 ); },
			[ indent ]( const std::unique_ptr< Primary >& expression ) { visit( *expression, indent + 1 ); }
		}, node.value );
	}

	static void visit( const ExpressionStatement& node, int indent ) {
		std::cout << indentText( indent, "ExpressionStatement" ) << std::endl;

		visit( *( node.value ), indent + 1 );
	}

	static void visit( const Statement& node, int indent ) {
		std::cout << indentText( indent, "Statement" ) << std::endl;

		visit( *( node.value ), indent + 1 );
	}

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