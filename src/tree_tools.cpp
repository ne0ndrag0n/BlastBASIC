#include "tree_tools.hpp"
#include <variant>

namespace GoldScorpion {

    std::optional< std::string > getIdentifierName( const Token& token ) {
        if( token.type == TokenType::TOKEN_IDENTIFIER && token.value ) {
            if( auto stringResult = std::get_if< std::string >( &*token.value ) ) {
                return *stringResult;
            }
        }

        return {};
    }

	std::optional< std::string > getIdentifierName( const Expression& node ) {
        if( auto primaryResult = std::get_if< std::unique_ptr< Primary > >( &node.value ) ) {
            const Primary& primary = **primaryResult;
            if( auto tokenResult = std::get_if< Token >( &primary.value ) ) {
                const Token& token = *tokenResult;
                return getIdentifierName( token );
            }
        }

        return {};
	}

    bool containsReturn( const WhileStatement& node ) {
        for( const auto& declaration : node.body ) {
            if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                const std::unique_ptr< Statement >& statement = *result;

                if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                    return true;
                }

                if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    // The IfStatement must return in any condition for this to return true. This means:
    // - There must be an "else" condition
    // - There must be a return statement in all conditions
    bool containsReturn( const IfStatement& node ) {
        // The IfStatement cannot guarantee a return if there are not more bodies than conditions
        // AKA there is no "else" statement
        if( node.bodies.size() <= node.conditions.size() ) {
            return false;
        }

        bool result = false;
        for( const auto& conditionBody : node.bodies ) {
            bool conditionBodyResult = false;

            for( const auto& declaration : conditionBody ) {
                if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                    const std::unique_ptr< Statement >& statement = *result;

                    if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                        conditionBodyResult = true; break;
                    }

                    if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                        if( containsReturn( **result ) ) {
                            conditionBodyResult = true; break;
                        }
                    }

                    if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                        if( containsReturn( **result ) ) {
                            conditionBodyResult = true; break;
                        }
                    }

                    if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                        if( containsReturn( **result ) ) {
                            conditionBodyResult = true; break;
                        }
                    }
                }
            }

            // If it returns false, don't waste any more time
            if( !conditionBodyResult ) {
                return false;
            } else {
                result = true;
            }
        }

        return result;
    }

    bool containsReturn( const ForStatement& node ) {
        for( const auto& declaration : node.body ) {
            if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                const std::unique_ptr< Statement >& statement = *result;

                if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                    return true;
                }

                if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    /**
     * Search for return statements in a function body, in the following method:
     * - Return statement occurs directly in the function body
     * - Return statement occurs anywhere in *all* conditions of an IfStatement
     * - Return statement occurs anywhere in a ForLoop
     * - Return statement occurs anywhere in a WhileLoop
     */
    bool containsReturn( const FunctionDeclaration& node ) {
        for( const auto& declaration : node.body ) {
            if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                const std::unique_ptr< Statement >& statement = *result;

                // Return statement was encountered directly on the function body
                if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                    return true;
                }

                // Check inside all conditions of one of the following statements
                if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

}