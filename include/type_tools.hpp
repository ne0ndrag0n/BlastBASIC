#pragma once
#include "token.hpp"
#include "memory_tracker.hpp"
#include "ast.hpp"
#include <string>
#include <optional>

namespace GoldScorpion {

    TokenType typeIdToTokenType( const std::string& id );

    std::string tokenTypeToTypeId( const TokenType type );

    std::optional< std::string > getType( const Primary& node, MemoryTracker& memory );

    std::optional< std::string > getType( const BinaryExpression& node, MemoryTracker& memory );

    std::optional< std::string > getType( const Expression& expression, MemoryTracker& memory );

}