#pragma once
#include "token.hpp"
#include "memory_tracker.hpp"
#include "result_type.hpp"
#include "ast.hpp"
#include <string>
#include <optional>

namespace GoldScorpion {

    using TypeResult = Result< MemoryDataType, std::string >;

    std::optional< TokenType > typeIdToTokenType( const std::string& id );

    std::optional< std::string > tokenTypeToTypeId( const TokenType type );

    std::optional< std::string > tokenToTypeId( const Token& token );

    std::string unwrapTypeId( const MemoryDataType& type );

    std::string typeToString( const MemoryDataType& type );

    bool typesComparable( const MemoryDataType& lhs, const MemoryDataType& rhs );

    bool typeIsValue( const MemoryDataType& type );

    bool typeIsFunction( const MemoryDataType& type );

    bool typeIsUdt( const MemoryDataType& type );

    bool typeIsInteger( const MemoryDataType& typeId );

    bool typeIsString( const MemoryDataType& type );

    bool typesMatch( const MemoryDataType& lhs, const MemoryDataType& rhs );

    bool integerTypesMatch( const MemoryDataType& lhs, const MemoryDataType& rhs );

    bool assignmentCoercible( const MemoryDataType& lhs, const MemoryDataType& rhs );

    bool coercibleToString( const MemoryDataType& lhs, const MemoryDataType& rhs );

    std::optional< long > getPrimitiveTypeSize( const MemoryDataType& type );

    std::optional< long > getUdtTypeSize( const MemoryDataType& type, const MemoryTracker& memory );

    // Only valid for integer fields
    MemoryDataType promotePrimitiveTypes( const MemoryDataType& lhs, const MemoryDataType& rhs );

    TypeResult getType( const Primary& node, MemoryTracker& memory );

    TypeResult getType( const CallExpression& node, MemoryTracker& memory );

    TypeResult getType( const BinaryExpression& node, MemoryTracker& memory );

    TypeResult getType( const Expression& expression, MemoryTracker& memory );

}