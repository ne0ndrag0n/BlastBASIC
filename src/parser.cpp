#include "parser.hpp"
#include "token.hpp"
#include "variant_visitor.hpp"
#include "ast.hpp"

namespace GoldScorpion {

	static AstResult< Expression > getExpression( std::vector< Token >::iterator current ) {
		return {};
	}

	static AstResult< Primary > getPrimary( std::vector< Token >::iterator current ) {
		Token currentToken = *current;

		switch( currentToken.type ) {
			case TokenType::TOKEN_THIS:
			case TokenType::TOKEN_LITERAL_INTEGER:
			case TokenType::TOKEN_LITERAL_STRING:
			case TokenType::TOKEN_IDENTIFIER:
				return GeneratedAstNode< Primary >{
					++current,
					std::make_unique< Primary >( Primary { currentToken } )
				};
			case TokenType::TOKEN_LEFT_PAREN: {
				// Attempt to get expression
				AstResult< Expression > expression = getExpression( ++current );
				if( expression ) {
					// Result valid if there is a closing paren in the next iterator
					if( expression->nextIterator->type == TokenType::TOKEN_RIGHT_PAREN ) {
						// Eat the current param and return the expression wrapped in a primary
						return GeneratedAstNode< Primary >{
							++expression->nextIterator,
							std::make_unique< Primary >( Primary { std::move( expression->node ) } )
						};
					}
				}

				// If we couldn't get an expression then return empty
				break;
			}
			default: {
				if( currentToken.type == TokenType::TOKEN_SUPER ) {
					// Next token must be a dot
					currentToken = *( ++current );

					if( currentToken.type == TokenType::TOKEN_DOT ) {
						// Next token must be an identifier
						currentToken = *( ++current );

						if( currentToken.type == TokenType::TOKEN_IDENTIFIER ) {
							// This is a BinaryExpression with super at left and IDENTIFIER at right
							std::unique_ptr< Primary > primary = std::make_unique< Primary >( Primary{
								std::make_unique< Expression >( Expression{
									std::make_unique< BinaryExpression >( BinaryExpression {

										std::make_unique< Expression >( Expression{
											std::make_unique< Primary >( Primary{
												Token{ TokenType::TOKEN_SUPER, {} }
											} )
										} ),

										std::make_unique< Primary >( Primary {
											Token{ TokenType::TOKEN_DOT, {} }
										} ),

										std::make_unique< Expression >( Expression{
											std::make_unique< Primary >( Primary{
												currentToken
											} )
										} )

									} )
								} )
							 } );

							return GeneratedAstNode< Primary >{ ++current, std::move( primary ) };
						}
					}
				}
			}
		}

		return {};
	}

	Result< Program > getProgram( std::vector< Token > filename ) {
		return "Not implemented";
	}
}