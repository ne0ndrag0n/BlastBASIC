#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <optional>
#include <string>

namespace GoldScorpion {

	std::optional< std::string > getIdentifierName( const Expression& node );

	std::optional< std::string > getIdentifierName( const Token& token );

	bool containsReturn( const WhileStatement& node );

	bool containsReturn( const IfStatement& node );

	bool containsReturn( const ForStatement& node );

	bool containsReturn( const FunctionDeclaration& node );
}