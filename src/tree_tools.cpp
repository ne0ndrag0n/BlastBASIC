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

}