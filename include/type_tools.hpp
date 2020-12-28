#pragma once
#include "token.hpp"
#include "memory_tracker.hpp"
#include "result_type.hpp"
#include "ast.hpp"
#include <string>
#include <optional>

namespace GoldScorpion {

    using TypeResult = Result< std::string, std::string >;

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

    TypeResult getType( const Primary& node, MemoryTracker& memory );

    TypeResult getType( const BinaryExpression& node, MemoryTracker& memory );

    TypeResult getType( const Expression& expression, MemoryTracker& memory );

}