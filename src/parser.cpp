#include "parser.hpp"
#include "token.hpp"
#include "variant_visitor.hpp"
#include "ast.hpp"
#include <exception>
#include <queue>

namespace GoldScorpion {

	static AstResult< Expression > getExpression( std::vector< Token >::iterator current ) {
		return {};
	}

	static AstResult< Expression > getPrimary( std::vector< Token >::iterator current ) {
		Token currentToken = *current;

		switch( currentToken.type ) {
			case TokenType::TOKEN_THIS:
			case TokenType::TOKEN_LITERAL_INTEGER:
			case TokenType::TOKEN_LITERAL_STRING:
			case TokenType::TOKEN_IDENTIFIER:
				return GeneratedAstNode< Expression >{
					++current,
					std::make_unique< Expression >( Expression {
						std::make_unique< Primary >( Primary { currentToken } )
					} )
				};
			case TokenType::TOKEN_LEFT_PAREN: {
				// Attempt to get expression
				AstResult< Expression > expression = getExpression( ++current );
				if( expression ) {
					// Result valid if there is a closing paren in the next iterator
					if( expression->nextIterator->type == TokenType::TOKEN_RIGHT_PAREN ) {
						// Eat the current param and return the expression wrapped in a primary
						return GeneratedAstNode< Expression >{
							++expression->nextIterator,
							std::make_unique< Expression >( Expression{
								std::make_unique< Primary >( Primary {
									std::move( expression->node )
								} )
							} )
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
							std::unique_ptr< Expression > expression = std::make_unique< Expression >( Expression{
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
							} );

							return GeneratedAstNode< Expression >{ ++current, std::move( expression ) };
						}
					}
				}
			}
		}

		// If at any moment we fail the process, return a null optional
		return {};
	}

	static AstResult< Expression > getCall( std::vector< Token >::iterator current ) {
		// Attempt to get a primary
		AstResult< Expression > primary = getPrimary( current );
		if( primary ) {
			current = primary->nextIterator;

			// Zero or more of either argument list or dot-identifier
			std::queue< std::unique_ptr< Expression > > queue;
			while( true ) {
				if( current->type == TokenType::TOKEN_LEFT_PAREN ) {
					// Need to parse an argument list

					// Eat the left paren token
					current++;

					// Begin eating arguments in the form of expressions separated by commas
					std::vector< std::unique_ptr< Expression > > arguments;
					while( AstResult< Expression > firstExpression = getExpression( current ) ) {
						current = firstExpression->nextIterator;
						arguments.emplace_back( std::move( firstExpression->node ) );

						// Keep eating expressions while a comma is present
						while( current->type != TokenType::TOKEN_COMMA ) {
							current++;

							if( AstResult< Expression > expression = getExpression( current ) ) {
								current = firstExpression->nextIterator;
								arguments.emplace_back( std::move( expression->node ) );
							} else {
								// Error if an expression doesn't follow a comma
								throw new std::runtime_error( "Expected: expression following a \",\"" );
							}
						}
					}

					// There better be a right paren to close
					if( current->type == TokenType::TOKEN_RIGHT_BRACKET ) {
						// Eat current
						current++;

						// Assemble CallExpression from current list of arguments 
						queue.emplace( std::make_unique< Expression >( Expression{
							std::make_unique< CallExpression >( CallExpression{
								nullptr,
								std::move( arguments )
							} )
						} ) );
					} else {
						throw new std::runtime_error( "Expected: closing \")\"" );
					}

				} else if( current->type == TokenType::TOKEN_DOT ) {
					if( AstResult< Expression > nextExpression = getPrimary( current ) ) {
						current = nextExpression->nextIterator;

						// Primary must be primary and identifier type
						if( auto primaryPtr = std::get_if< std::unique_ptr< Primary > >( &nextExpression->node->value ) ) {
							// Now the primary must be both a token and identifier token
							if( auto tokenResult = std::get_if< Token >( &( *primaryPtr )->value ) ) {
								if( tokenResult->type == TokenType::TOKEN_IDENTIFIER ) {
									// Move nextExpression onto the queue
									queue.emplace( std::move( nextExpression->node ) );
								} else {
									throw new std::runtime_error( "Expected: token of IDENTIFIER type" );
								}
							} else {
								throw new std::runtime_error( "Expected: primary of token type" );
							}
						} else {
							throw new std::runtime_error( "Expected: primary" );
						}
					}
				} else {
					break;
				}
			}

			// Begin building the tree from treeStack
			while( !queue.empty() ) {
				// Queue will contain either null function calls or identifiers
				// When encountering a call: primary = call with current primary as identifier
				// When encounering an identifier: primary = dot with lhs primary and rhs identifier
				if( auto result = std::get_if< std::unique_ptr< CallExpression > >( &queue.front()->value ) ) {
					(*result)->identifier = std::move( primary->node ); 
					primary->node = std::make_unique< Expression >( Expression {
						std::move( *result )
					} );
				} else if( auto result = std::get_if< std::unique_ptr< Primary > >( &queue.front()->value ) ) {
					std::unique_ptr< BinaryExpression > binary = std::make_unique< BinaryExpression >( BinaryExpression {
						std::move( primary->node ),

						std::make_unique< Primary >( Primary {
							Token{ TokenType::TOKEN_DOT, {} }
						} ),

						std::make_unique< Expression >( Expression {
							std::move( *result )
						} )
					} );

					primary->node = std::make_unique< Expression >( Expression {
						std::move( binary )
					} );
				} else {
					throw new std::runtime_error( "Internal compiler error (unexpected item in call-expression queue)" );
				}

				queue.pop();
			}

			return primary;
		}

		return {};
	}

	Result< Program > getProgram( std::vector< Token > filename ) {
		return "Not implemented";
	}
}