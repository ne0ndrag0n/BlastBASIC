#pragma once
#include "token.hpp"
#include "variant_visitor.hpp"
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <utility>

namespace GoldScorpion {

	template< typename ReturnType >
	struct GeneratedAstNode {
		std::vector< Token >::iterator nextIterator;
		std::unique_ptr< ReturnType > node;
	};

	template< typename ReturnType >
	using AstResult = std::optional< GeneratedAstNode< ReturnType > >;

	struct DataType {
		Token type;
		std::optional< Token > arraySize;

		std::string toString() const {
			std::string typeString = type.toString();
			if( arraySize ) {
				typeString += " Size: " + arraySize->toString();
			}

			return typeString;
		}
	};

	struct Primary {
		std::variant< Token, std::unique_ptr< struct Expression > > value;
	};

	struct CallExpression {
		std::unique_ptr< Expression > identifier;
		std::vector< std::unique_ptr< struct Expression > > arguments;
	};

	struct UnaryExpression {
		std::unique_ptr< Primary > op;
		std::unique_ptr< struct Expression > value;
	};

	struct BinaryExpression {
		std::unique_ptr< struct Expression > lhsValue;
		std::unique_ptr< Primary > op;
		std::unique_ptr< struct Expression > rhsValue;
	};

	struct AssignmentExpression {
		std::unique_ptr< struct Expression > identifier;
		std::unique_ptr< struct Expression > expression;
	};

	struct Expression {
		std::variant<
			std::unique_ptr< AssignmentExpression >,
			std::unique_ptr< BinaryExpression >,
			std::unique_ptr< UnaryExpression >,
			std::unique_ptr< CallExpression >,
			std::unique_ptr< Primary >
		> value;
	};

	struct ExpressionStatement {
		std::unique_ptr< Expression > value;
	};

	struct ForStatement {
		Token index;
		std::unique_ptr< Expression > from;
		std::unique_ptr< Expression > to;
		std::optional< std::unique_ptr< Expression > > every;
		std::vector< std::unique_ptr< struct Declaration > > body;
	};

	struct IfStatement {
		std::vector< std::unique_ptr< Expression > > conditions;
		std::vector< std::vector< std::unique_ptr< struct Declaration > > > bodies;
	};

	struct ReturnStatement {
		std::optional< std::unique_ptr< Expression > > expression;
	};

	struct AsmStatement {
		Token body;
	};

	struct WhileStatement {
		std::unique_ptr< Expression > condition;
		std::vector< std::unique_ptr< struct Declaration > > body;
	};

	struct Statement {
		std::variant<
			std::unique_ptr< ExpressionStatement >,
			std::unique_ptr< ForStatement >,
			std::unique_ptr< IfStatement >,
			std::unique_ptr< ReturnStatement >,
			std::unique_ptr< AsmStatement >,
			std::unique_ptr< WhileStatement >
		> value;
	};

	struct Parameter {
		Token name;
		DataType type;
	};

	struct VarDeclaration {
		Parameter variable;
		std::optional< std::unique_ptr< Expression > > value;
	};

	struct ConstDeclaration {
		Parameter variable;
		std::unique_ptr< Expression > value;
	};

	struct FunctionDeclaration {
		std::optional< Token > name;
		std::vector< Parameter > arguments;
		std::optional< Token > returnType;
		std::vector< std::unique_ptr< struct Declaration > > body;
	};

	struct TypeDeclaration {
		Token name;
		std::vector< Parameter > fields;
		std::vector< std::unique_ptr< FunctionDeclaration > > functions;
	};

	struct ImportDeclaration {
		std::string path;
	};

	struct Annotation {
		std::vector< std::unique_ptr< Expression > > directives;
	};

	struct Declaration {
		std::variant<
			std::unique_ptr< Annotation >,
			std::unique_ptr< VarDeclaration >,
			std::unique_ptr< ConstDeclaration >,
			std::unique_ptr< FunctionDeclaration >,
			std::unique_ptr< TypeDeclaration >,
			std::unique_ptr< ImportDeclaration >,
			std::unique_ptr< Statement >
		> value;
	};

	struct Program {
		std::vector< std::unique_ptr< Declaration > > statements;
	};
}