#include "symbol.hpp"
#include "variant_visitor.hpp"

namespace GoldScorpion {

    std::string getSymbolId( const Symbol& symbol ) {
        return std::visit( overloaded {
            []( const VariableSymbol& symbol ) {
                return symbol.id;
            },
            []( const ConstantSymbol& symbol ) {
                return symbol.id;
            },
            []( const FunctionSymbol& symbol ) {
                return symbol.id;
            },
            []( const UdtSymbol& symbol ) {
                return symbol.id;
            },
        }, symbol.symbol );
    }

    std::optional< SymbolTable > SymbolResolver::getByFileId( const std::string& id ) {
        for( const SymbolTable& symbolTable : symbolTables ) {
            if( symbolTable.fileId == id ) {
                return symbolTable;
            }
        }

        return {};
    }

    void SymbolResolver::addFile( const std::string& id ) {
        if( getByFileId( id ) ) {
            // Cannot double-add a file - did you do something wrong?
            return;
        }

        SymbolTable symbolTable;
        symbolTable.fileId = id;

        symbolTables.push_back( symbolTable );
    }

    void SymbolResolver::addOuterScope( const std::string& id, const std::string& outerScopeId ) {
        if( auto symbolTable = getByFileId( id ) ) {
            symbolTable->outerScopes.push_back( outerScopeId );
        }
    }

    std::optional< Symbol > SymbolResolver::findSymbol( const std::string& fileId, const std::string& symbolId ) {
        std::optional< SymbolTable > symbolTable = getByFileId( fileId );
        if( !symbolTable ) {
            return {};
        }

        // Step 1: Search scopes from top of stack down
        std::stack< std::vector< Symbol > > scopes = symbolTable->scopes;
        while( !scopes.empty() ) {
            const std::vector< Symbol >& scope = scopes.top();
            for( const Symbol& symbol : scope ) {
                if( getSymbolId( symbol ) == symbolId ) {
                    return symbol;
                }
            }

            scopes.pop();
        }

        // Step 2: Search symbols in own file
        for( const Symbol& symbol : symbolTable->symbols ) {
            if( getSymbolId( symbol ) == symbolId ) {
                return symbol;
            }
        }

        // Step 3: Search public symbols in all outer scopes (files)
        for( const std::string& outerScope : symbolTable->outerScopes ) {
            if( auto externalSymbolTable = getByFileId( outerScope ) ) {
                for( const Symbol& symbol : externalSymbolTable->symbols ) {
                    if( symbol.external && getSymbolId( symbol ) == symbolId ) {
                        return symbol;
                    }
                }
            }
        }

        // The symbol wasn't found in the given context
        return {};
    }

    void SymbolResolver::addSymbol( const std::string& fileId, Symbol symbol ) {
        if( auto symbolTable = getByFileId( fileId ) ) {
            // If there are any scopes open, add to the scope
            // Otherwise add to the file symbols
            if( !symbolTable->scopes.empty() ) {
                symbolTable->scopes.top().push_back( symbol );
            } else {
                symbolTable->symbols.push_back( symbol );
            }
        }
    }

    void SymbolResolver::openScope( const std::string& fileId ) {
        if( auto symbolTable = getByFileId( fileId ) ) {
            symbolTable->scopes.push( std::vector< Symbol >{} );
        }
    }

    std::vector< Symbol > SymbolResolver::closeScope( const std::string& fileId ) {
        if( auto symbolTable = getByFileId( fileId ) ) {
            if( !symbolTable->scopes.empty() ) {
                std::vector< Symbol > symbols = symbolTable->scopes.top();
                symbolTable->scopes.pop();
                return symbols;
            }
        }

        return std::vector< Symbol >{};
    }

}