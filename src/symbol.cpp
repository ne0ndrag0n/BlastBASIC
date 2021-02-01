#include "symbol.hpp"
#include "variant_visitor.hpp"
#include "type_tools.hpp"

namespace GoldScorpion {

    std::vector< SymbolType > SymbolResolver::handles;

    SymbolType SymbolResolver::toSymbolType( SymbolTypeHandle handle ) {
        return handles[ handle ];
    }

    SymbolTypeHandle SymbolResolver::addSymbolType( SymbolType incoming ) {
        for( size_t i = 0; i != handles.size(); i++ ) {
            const SymbolType& type = handles[ i ];
            if( incoming.index() == type.index() ) {
                // Cheap way to do this is to just compare the type id which will be equal for equivalent types
                if( getSymbolTypeId( incoming ) == getSymbolTypeId( type ) ) {
                    return i;
                }
            }
        }

        handles.push_back( incoming );
        return handles.size() - 1;
    }

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

    std::string getSymbolTypeId( const SymbolType& symbolType ) {
         return std::visit( overloaded{
            [ & ]( const SymbolNativeType& type ) {
                return *tokenTypeToTypeId( type.type );
            },
            [ & ]( const SymbolUdtType& type ) {
                return type.id;
            },
            [ & ]( const SymbolFunctionType& type ) {
                return type.id;
            },
            [ & ]( const SymbolArrayType& type ) {
                std::string dimensionString = getSymbolTypeId( toSymbolType( type.base ) ) +  "[";

                for( size_t i = 0; i != type.dimensions.size(); i++ ) {
                    dimensionString += std::to_string( type.dimensions[ i ] );
                    if( i != type.dimensions.size() - 1 ) {
                        dimensionString += ",";
                    }
                }
                dimensionString += "]";

                return dimensionString;
            }
        }, symbolType );
    }

    bool fieldPresent( const std::string& fieldId, const UdtSymbol& symbol ) {
        for( const SymbolField& field : symbol.fields ) {
            if( field.id == fieldId ) {
                return true;
            }
        }

        return false;
    }

    SymbolTable* SymbolResolver::getByFileId( const std::string& id ) {
        for( SymbolTable& symbolTable : symbolTables ) {
            if( symbolTable.fileId == id ) {
                return &symbolTable;
            }
        }

        return nullptr;
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

    Symbol* SymbolResolver::getSymbol( const std::string& fileId, const std::string& symbolId ) {
        SymbolTable* symbolTable = getByFileId( fileId );
        if( !symbolTable ) {
            return nullptr;
        }

        // Step 1: Search scopes from top of stack down
        for( auto it = symbolTable->scopes.rbegin(); it != symbolTable->scopes.rend(); ++it ) {
            std::vector< Symbol >& scope = *it;
            for( Symbol& symbol : scope ) {
                if( getSymbolId( symbol ) == symbolId ) {
                    return &symbol;
                }
            }
        }

        // Step 2: Search symbols in own file
        for( Symbol& symbol : symbolTable->symbols ) {
            if( getSymbolId( symbol ) == symbolId ) {
                return &symbol;
            }
        }

        // Step 3: Search public symbols in all outer scopes (files)
        for( const std::string& outerScope : symbolTable->outerScopes ) {
            if( auto externalSymbolTable = getByFileId( outerScope ) ) {
                for( Symbol& symbol : externalSymbolTable->symbols ) {
                    if( symbol.external && getSymbolId( symbol ) == symbolId ) {
                        return &symbol;
                    }
                }
            }
        }

        // The symbol wasn't found in the given context
        return nullptr;
    }

    std::optional< Symbol > SymbolResolver::findSymbol( const std::string& fileId, const std::string& symbolId ) {
        Symbol* symbol = getSymbol( fileId, symbolId );

        if( symbol ) {
            // Copy
            return *symbol;
        } else {
            return {};
        }
    }

    void SymbolResolver::addSymbol( const std::string& fileId, Symbol symbol ) {
        if( auto symbolTable = getByFileId( fileId ) ) {
            // If there are any scopes open, add to the scope
            // Otherwise add to the file symbols
            if( !symbolTable->scopes.empty() ) {
                symbolTable->scopes.back().push_back( symbol );
            } else {
                symbolTable->symbols.push_back( symbol );
            }
        }
    }

    void SymbolResolver::addFieldToSymbol( const std::string& fileId, const std::string& symbolId, SymbolField field ) {
        if( auto query = getSymbol( fileId, symbolId ) ) {
            if( auto asUdt = std::get_if< UdtSymbol >( &( query->symbol ) ) ) {
                asUdt->fields.push_back( field );
            }
        }
    }

    void SymbolResolver::openScope( const std::string& fileId ) {
        if( auto symbolTable = getByFileId( fileId ) ) {
            symbolTable->scopes.push_back( std::vector< Symbol >{} );
        }
    }

    std::vector< Symbol > SymbolResolver::closeScope( const std::string& fileId ) {
        if( auto symbolTable = getByFileId( fileId ) ) {
            if( !symbolTable->scopes.empty() ) {
                std::vector< Symbol > symbols = symbolTable->scopes.back();
                symbolTable->scopes.pop_back();
                return symbols;
            }
        }

        return std::vector< Symbol >{};
    }

    SymbolType toSymbolType( const ArrayIntermediateType& type ) {
       if( auto unwrap = std::get_if< SymbolNativeType >( &type ) ) {
           return *unwrap;
       } else if( auto unwrap = std::get_if< SymbolUdtType >( &type ) ) {
           return *unwrap;
       } else if( auto unwrap = std::get_if< SymbolFunctionType >( &type ) ) {
           return *unwrap;
       } else {
           Error{ "Internal compiler error (unknown type in ArrayIntermediateType)", {} }.throwException();
           throw "";
       }
    }

    ArrayIntermediateType toArrayIntermediateType( const SymbolType& type ) {
        ArrayIntermediateType arrayType;

        std::visit( overloaded {
            [ &arrayType ]( const SymbolNativeType& type ) { arrayType = type; },
            [ &arrayType ]( const SymbolFunctionType& type ) { arrayType = type; },
            [ &arrayType ]( const SymbolUdtType& type ) { arrayType = type; },
            []( const SymbolArrayType& ) { Error{ "Internal compiler error (cannot wrap an array in an array intermediate type)", {} }.throwException(); }
        }, type );

        return arrayType;
    }

}