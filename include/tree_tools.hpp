#pragma once
#include "ast.hpp"
#include "token.hpp"
#include "symbol.hpp"
#include <optional>
#include <string>
#include <vector>
#include <stack>
#include <variant>

namespace GoldScorpion {

	struct ConstEvaluationSettings {
		std::string fileId;
		std::stack< ConstantExpressionValue >& stack;
		SymbolResolver& symbols;
		std::optional< Token > nearestToken;
	};

	std::optional< std::string > getIdentifierName( const Expression& node );

	std::optional< std::string > getIdentifierName( const Token& token );

	bool containsReturn( const WhileStatement& node );

	bool containsReturn( const IfStatement& node );

	bool containsReturn( const ForStatement& node );

	bool containsReturn( const FunctionDeclaration& node );

	ConstantExpressionValue evaluateConst( const Expression& node, ConstEvaluationSettings settings );
}