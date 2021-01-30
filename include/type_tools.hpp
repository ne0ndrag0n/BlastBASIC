#pragma once
#include "token.hpp"
#include "result_type.hpp"
#include "symbol.hpp"
#include "ast.hpp"
#include <string>
#include <optional>

namespace GoldScorpion {

    struct SymbolTypeSettings {
        std::string fileId;
        SymbolResolver& symbols;
    };
    using SymbolTypeResult = Result< SymbolType, std::string >;

    std::optional< TokenType > typeIdToTokenType( const std::string& id );

    std::optional< std::string > tokenTypeToTypeId( const TokenType type );

    std::optional< std::string > tokenToTypeId( const Token& token );

    bool tokenIsPrimitiveType( const Token& token );

    bool typesComparable( const SymbolType& lhs, const SymbolType& rhs );

    bool typeIsArray( const SymbolType& type );

    bool typeIsFunction( const SymbolType& type );

    bool typeIsUdt( const SymbolType& type );

    bool typeIsInteger( const SymbolType& type );

    bool typeIsString( const SymbolType& type );

    bool typesMatch( const SymbolType& lhs, const SymbolType& rhs, SymbolTypeSettings settings );

    bool integerTypesMatch( const SymbolType& lhs, const SymbolType& rhs );

    bool assignmentCoercible( const SymbolType& lhs, const SymbolType& rhs );

    bool coercibleToString( const SymbolType& lhs, const SymbolType& rhs );

    SymbolNativeType promotePrimitiveTypes( const SymbolNativeType& lhs, const SymbolNativeType& rhs );

    // New symbol type stuff that will replace the type stuff immediately above
    SymbolTypeResult getType( const Primary& node, SymbolTypeSettings settings );
    SymbolTypeResult getType( const CallExpression& node, SymbolTypeSettings settings );
    SymbolTypeResult getType( const BinaryExpression& node, SymbolTypeSettings settings );
    SymbolTypeResult getType( const AssignmentExpression& node, SymbolTypeSettings settings );
    SymbolTypeResult getType( const Expression& node, SymbolTypeSettings settings );

}
