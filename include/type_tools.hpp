#pragma once
#include "token.hpp"
#include "memory_tracker.hpp"
#include "ast.hpp"
#include <string>
#include <optional>

namespace GoldScorpion {

    std::optional< TokenType > typeIdToTokenType( const std::string& id );

    std::optional< std::string > tokenTypeToTypeId( const TokenType type );

    std::optional< std::string > tokenToTypeId( const Token& token );

    bool typeIsUdt( const std::string& typeId );

    bool typeIsInteger( const std::string& typeId );

    bool typesMatch( const std::string& lhs, const std::string& rhs );

    bool integerTypesMatch( const std::string& lhs, const std::string& rhs );

    bool coercibleToString( const std::string& lhs, const std::string& rhs );

    // Only valid for integer fields
    std::string promotePrimitiveTypes( const std::string& lhs, const std::string& rhs );

    std::optional< std::string > getType( const Primary& node, MemoryTracker& memory );

    std::optional< std::string > getType( const BinaryExpression& node, MemoryTracker& memory );

    std::optional< std::string > getType( const Expression& expression, MemoryTracker& memory );

}