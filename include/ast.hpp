#pragma once
#include "tokens.hpp"
#include <memory>
#include <vector>
#include <variant>
#include <string>
#include <optional>
#include <utility>

namespace GoldScorpion {

	using Identifier = std::string;
	enum class PrimitiveType { U8, U16, U32, S8, S16, S32, STRING };

	struct DataType {
		std::variant< PrimitiveType, Identifier > type;
		unsigned int size = 0;		// Used for either array type or string annotation
									// Size 0 for non-strings = Single type
									// Size 0 for strings = compiler error
	};

	using NumericLiteral = long;
	using StringLiteral = std::string;
	using Expression = std::variant<
		std::unique_ptr< struct UnaryExpression >,
		std::unique_ptr< struct BinaryExpression >,
		std::unique_ptr< struct AssignmentExpression >,
		NumericLiteral,
		StringLiteral
	>;

	struct UnaryExpression {
		Token op;
		Expression expression;
	};

	struct BinaryExpression {
		Expression left;
		Token op;
		Expression right;
	};

	struct AssignmentExpression {
		Identifier identifier;
		Expression expression;
	};

	struct DefineDeclaration {
		Identifier identifier;
		DataType type;
		std::optional< std::vector< Expression > > initializers;
	};

	using Statement = std::variant<
		Expression,
		std::unique_ptr< struct DefineDeclaration >,
		std::unique_ptr< struct FunctionDeclaration >,
		std::unique_ptr< struct IfStatement >,
		std::unique_ptr< struct ReturnStatement >,
		std::unique_ptr< struct TypeDeclaration >,
		std::unique_ptr< struct ImportStatement >,
		std::unique_ptr< struct InlineAsmStatement >
	>;

	struct IfStatement {
		std::pair< Expression, std::vector< Statement > > primaryCondition;
		std::vector< std::pair< Expression, std::vector< Statement > > > elifConditions;
		std::optional< std::vector< Statement > > elseCondition;
	};

	struct ArgumentDeclaration {
		Identifier name;
		DataType type;
		std::optional< bool > byref = false;
	};

	struct FunctionDeclaration {
		Identifier name;
		std::vector< ArgumentDeclaration > arguments;
		DataType returnType;
		std::vector< Statement > body;
	};

	struct ReturnStatement {
		Expression expression;
	};

	struct TypeDeclaration {
		std::vector< ArgumentDeclaration > fields;
		std::vector< FunctionDeclaration > memberFunctions;
	};

	struct ImportStatement {
		std::string path;
	};

	struct InlineAsmStatement {
		std::string body;
	};

	struct Program {
		std::vector< Statement > statements;
	};
}