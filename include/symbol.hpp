#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <stack>

namespace GoldScorpion {

    struct SymbolNativeType { TokenType type; };
    struct SymbolUdtType { std::string id; };
    struct SymbolFunctionType { std::string id; };
    using SymbolType = std::variant< SymbolNativeType, SymbolFunctionType, SymbolUdtType >;

    struct SymbolArgument { std::string id; SymbolType type; };

    struct VariableSymbol { std::string id; SymbolType type; };
    struct ConstantSymbol { std::string id; SymbolType type; };
    struct FunctionSymbol { std::string id; std::vector< SymbolArgument > arguments; std::optional< SymbolType > functionReturnType; };
    struct UdtSymbol { std::string id; std::vector< SymbolArgument > fields; };
    struct Symbol {
        std::variant< VariableSymbol, ConstantSymbol, FunctionSymbol, UdtSymbol > symbol;
        bool external = false;
    };

    struct SymbolTable {
        std::string fileId;
        std::vector< std::string > outerScopes;
        std::stack< std::vector< Symbol > > scopes;
        std::vector< Symbol > symbols;
    };

    class SymbolResolver {
        std::vector< SymbolTable > symbolTables;

        std::optional< SymbolTable > getByFileId( const std::string& id );

    public:
        void addFile( const std::string& id );
        void addOuterScope( const std::string& id, const std::string& outerScopeId );

        std::optional< Symbol > findSymbol( const std::string& fileId, const std::string& symbolId );
        void addSymbol( const std::string& fileId, Symbol symbol );

        void openScope( const std::string& fileId );
        std::vector< Symbol > closeScope( const std::string& fileId );

        static std::string getSymbolId( const Symbol& symbol );
    };

}