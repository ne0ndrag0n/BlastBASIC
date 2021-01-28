#pragma once
#include "ast.hpp"
#include "token.hpp"
#include "error.hpp"
#include "result_type.hpp"
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <memory>
#include <stack>
#include <cstddef>

namespace GoldScorpion {

    using ConstantExpressionValue = std::variant< long, std::string >;
    using SymbolTypeHandle = size_t;

    struct SymbolNativeType { TokenType type; };
    struct SymbolUdtType { std::string id; };
    struct SymbolFunctionType { std::string id; };
    struct SymbolArrayType { std::vector< long > dimensions; SymbolTypeHandle base; };
    using SymbolType = std::variant< SymbolNativeType, SymbolFunctionType, SymbolUdtType, SymbolArrayType >;

    using SymbolTypeResult = Result< SymbolType, std::string >;

    struct SymbolArgument { std::string id; SymbolType type; };

    struct VariableSymbol { std::string id; SymbolType type; };
    struct ConstantSymbol { std::string id; SymbolType type; ConstantExpressionValue value; };
    struct FunctionSymbol { std::string id; std::vector< SymbolArgument > arguments; std::optional< SymbolType > functionReturnType; };
    struct SymbolField {
        std::string id;
        std::variant< VariableSymbol, FunctionSymbol > value;
    };
    struct UdtSymbol { std::string id; std::vector< SymbolField > fields; };
    struct Symbol {
        std::variant< VariableSymbol, ConstantSymbol, FunctionSymbol, UdtSymbol > symbol;
        bool external = false;
    };

    struct SymbolTable {
        std::string fileId;
        std::vector< std::string > outerScopes;
        std::vector< std::vector< Symbol > > scopes;
        std::vector< Symbol > symbols;
    };

    class SymbolResolver {
        std::vector< SymbolTable > symbolTables;
        static std::vector< SymbolType > handles;

        SymbolTable* getByFileId( const std::string& id );
        Symbol* getSymbol( const std::string& fileId, const std::string& symbolId );

    public:
        void addFile( const std::string& id );
        void addOuterScope( const std::string& id, const std::string& outerScopeId );

        std::optional< Symbol > findSymbol( const std::string& fileId, const std::string& symbolId );
        void addSymbol( const std::string& fileId, Symbol symbol );
        void addFieldToSymbol( const std::string& fileId, const std::string& symbolId, SymbolField field );

        void openScope( const std::string& fileId );
        std::vector< Symbol > closeScope( const std::string& fileId );

        static SymbolType toSymbolType( SymbolTypeHandle handle );
        static SymbolTypeHandle addSymbolType( SymbolType incoming );
    };

    std::string getSymbolId( const Symbol& symbol );
    std::string getSymbolTypeId( const SymbolType& symbolType );
    bool fieldPresent( const std::string& fieldId, const UdtSymbol& symbol );
}