#include "parser.hpp"
#include "token.hpp"
#include "variant_visitor.hpp"
#include "ast.hpp"
#include <exception>
#include <optional>
#include <queue>

namespace GoldScorpion {

	// File-scope vars
	static std::vector< Token >::iterator end;

	// Forward declarations
	static AstResult< Expression > getExpression( std::vector< Token >::iterator current );
	// End forward declarations

	// Do not read iterator if it is past the end
	static std::optional< Token > readToken( std::vector< Token >::iterator iterator ) {
		if( iterator != end ) {
			return *iterator;
		}

		return {};
	}

	static AstResult< Expression > getPrimary( std::vector< Token >::iterator current ) {
		if( auto result = readToken( current ) ) {
			Token currentToken = *result;

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
						++current;
						if( auto result = readToken( current ) ) { currentToken = *result; } else { return {}; }

						if( currentToken.type == TokenType::TOKEN_DOT ) {
							// Next token must be an identifier
							++current;
							if( auto result = readToken( current ) ) { currentToken = *result; } else { return {}; }

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
				if( readToken( current ) && current->type == TokenType::TOKEN_LEFT_PAREN ) {
					// Need to parse an argument list

					// Eat the left paren token
					current++;

					// Begin eating arguments in the form of expressions separated by commas
					std::vector< std::unique_ptr< Expression > > arguments;
					while( AstResult< Expression > firstExpression = getExpression( current ) ) {
						current = firstExpression->nextIterator;
						arguments.emplace_back( std::move( firstExpression->node ) );

						// Keep eating expressions while a comma is present
						while( readToken( current ) && current->type != TokenType::TOKEN_COMMA ) {
							current++;

							if( AstResult< Expression > expression = getExpression( current ) ) {
								current = firstExpression->nextIterator;
								arguments.emplace_back( std::move( expression->node ) );
							} else {
								// Error if an expression doesn't follow a comma
								throw new std::runtime_error( "Expected: Expression following a \",\"" );
							}
						}
					}

					// There better be a right paren to close
					if( readToken( current ) && current->type == TokenType::TOKEN_RIGHT_BRACKET ) {
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

				} else if( readToken( current ) && current->type == TokenType::TOKEN_DOT ) {
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
									throw new std::runtime_error( "Expected: Token of IDENTIFIER type" );
								}
							} else {
								throw new std::runtime_error( "Expected: Primary of Token type" );
							}
						} else {
							throw new std::runtime_error( "Expected: Expression of Primary type" );
						}
					} else {
						throw new std::runtime_error( "Expected: Primary following \".\"" );
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

			primary->nextIterator = current;
			return primary;
		}

		return {};
	}

	static AstResult< Expression > getUnary( std::vector< Token >::iterator current ) {
		// If current is not a "not" token or a "-" token, then it is not a unary, go right to call
		if( readToken( current ) && ( current->type == TokenType::TOKEN_NOT || current->type == TokenType::TOKEN_MINUS ) ) {
			// Save token
			Token operatorToken = *current;

			// A terminal unary must follow
			AstResult< Expression > unary = getUnary( current );
			if( unary ) {
				return GeneratedAstNode< Expression >{
					unary->nextIterator,
					std::make_unique< Expression >( Expression{
						std::make_unique< UnaryExpression >( UnaryExpression {
							std::make_unique< Primary >( Primary{ operatorToken } ),

							std::move( unary->node )
						} )
					} )
				};
			} else {
				throw new std::runtime_error( "Expected: terminal Expression following unary operator" );
			}
		} else {
			return getCall( current );
		}
	}

	static AstResult< Expression > getFactor( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getUnary( current );
		if( result ) {
			// Keep taking unary expressions as long as the next token is a "/" or "*"
			current = result->nextIterator;
			while( readToken( current ) && ( current->type == TokenType::TOKEN_ASTERISK || current->type == TokenType::TOKEN_FORWARD_SLASH ) ) {
				Token op = *current;

				AstResult< Expression > next = getUnary( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression{
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} )
					} );

					result->node = std::move( binaryExpression );
				} else {
					throw new std::runtime_error( "Expected: terminal Unary following operator \"*\" or \"/\"" );
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getTerm( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getFactor( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && ( current->type == TokenType::TOKEN_MINUS || current->type == TokenType::TOKEN_PLUS ) ) {
				Token op = *current;

				AstResult< Expression > next = getFactor( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression{
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} )
					} );

					result->node = std::move( binaryExpression );
				} else {
					throw new std::runtime_error( "Expected: terminal Factor following operator \"-\" or \"+\"" );
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getComparison( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getTerm( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) &&
				 ( current->type == TokenType::TOKEN_GREATER_THAN ||
				   current->type == TokenType::TOKEN_GREATER_THAN_EQUAL ||
				   current->type == TokenType::TOKEN_LESS_THAN ||
				   current->type == TokenType::TOKEN_LESS_THAN_EQUAL ) ) {
				Token op = *current;

				AstResult< Expression > next = getTerm( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression{
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} )
					} );

					result->node = std::move( binaryExpression );
				} else {
					throw new std::runtime_error( "Expected: terminal Term following operator \">\", \">=\", \"<\", or \"<=\"" );
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getEquality( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getComparison( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && ( current->type == TokenType::TOKEN_NOT_EQUALS || current->type == TokenType::TOKEN_DOUBLE_EQUALS ) ) {
				Token op = *current;

				AstResult< Expression > next = getComparison( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression{
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} )
					} );

					result->node = std::move( binaryExpression );
				} else {
					throw new std::runtime_error( "Expected: terminal Comparison following operator \"!=\" or \"==\"" );
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getLogicAnd( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getEquality( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && current->type == TokenType::TOKEN_AND ) {
				Token op = *current;

				AstResult< Expression > next = getEquality( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression{
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} )
					} );

					result->node = std::move( binaryExpression );
				} else {
					throw new std::runtime_error( "Expected: terminal Equality following operator \"and\"" );
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getLogicXor( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getLogicAnd( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && current->type == TokenType::TOKEN_XOR ) {
				Token op = *current;

				AstResult< Expression > next = getLogicAnd( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression{
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} )
					} );

					result->node = std::move( binaryExpression );
				} else {
					throw new std::runtime_error( "Expected: terminal LogicAnd following operator \"xor\"" );
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getLogicOr( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getLogicXor( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && current->type == TokenType::TOKEN_OR ) {
				Token op = *current;

				AstResult< Expression > next = getLogicXor( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression{
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} )
					} );

					result->node = std::move( binaryExpression );
				} else {
					throw new std::runtime_error( "Expected: terminal LogicXor following operator \"or\"" );
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getAssignment( std::vector< Token >::iterator current ) {
		std::unique_ptr< Expression > prefix = nullptr;

		// Begin with zero or one call
		AstResult< Expression > call = getCall( current );
		if( call ) {
			current = call->nextIterator;

			// Next token better be a dot
			if( readToken( current ) && current->type == TokenType::TOKEN_DOT ) {
				current++;

				// Set result
				prefix = std::move( call->node );
			} else {
				throw new std::runtime_error( "Expected: dot following Call expression" );
			}
		}

		// Next two tokens are IDENTIFIER "=", or something else.
		if( AstResult< Expression > identifier = getPrimary( current ) ) {
			if( auto result = std::get_if< std::unique_ptr< Primary > >( &identifier->node->value ) ) {
				if( auto tokenResult = std::get_if< Token >( &( *result )->value ) ) {
					if( tokenResult->type == TokenType::TOKEN_IDENTIFIER ) {
						// Equals now must follow an identifier to be an AssignmentExpression

						if( AstResult< Expression > potentialEquals = getPrimary( identifier->nextIterator ) ) {
							if( auto potentialEqualsResult = std::get_if< std::unique_ptr< Primary > >( &potentialEquals->node->value ) ) {
								if( auto equalsResult = std::get_if< Token >( &( *potentialEqualsResult )->value ) ) {
									if( equalsResult->type == TokenType::TOKEN_EQUALS ) {
										// An assignment expression now must follow

										if( AstResult< Expression > assignmentResult = getAssignment( potentialEquals->nextIterator ) ) {
											// This is an assignment expression

											// If a prefix is present, the left hand side will be a BinaryExpression, dot, with
											// prefix on the LHS and IDENTIFIER on the RHS
											// Otherwise, the left hand side will be IDENTIFIER
											if( prefix ) {
												return GeneratedAstNode< Expression >{
													assignmentResult->nextIterator,
													std::make_unique< Expression >( Expression{
														std::make_unique< AssignmentExpression >( AssignmentExpression {
															std::make_unique< Expression >( Expression {
																std::make_unique< BinaryExpression >( BinaryExpression{
																	std::move( prefix ),

																	std::make_unique< Primary >( Primary {
																		Token{ TokenType::TOKEN_DOT, {} }
																	} ),

																	std::move( identifier->node )
																} )
															} ),
															std::move( assignmentResult->node )
														} )
													} )
												};
											} else {
												return GeneratedAstNode< Expression >{
													assignmentResult->nextIterator,
													std::make_unique< Expression >( Expression{
														std::make_unique< AssignmentExpression >( AssignmentExpression{
															std::move( identifier->node ),
															std::move( assignmentResult->node )
														} )
													} )
												};
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// If we get here then we want to fall through to a LogicOr expression
		return getLogicOr( current );
	}

	static AstResult< Expression > getExpression( std::vector< Token >::iterator current ) {
		return getAssignment( current );
	}

	static AstResult< ExpressionStatement > getExpressionStatement( std::vector< Token >::iterator current ) {
		AstResult< Expression > expressionResult = getExpression( current );
		if( expressionResult ) {
			current = expressionResult->nextIterator;

			// Validate return with a newline
			if( auto result = readToken( current ) ) {
				if( result->type == TokenType::TOKEN_NEWLINE ) {
					return GeneratedAstNode< ExpressionStatement >{
						++current,
						std::make_unique< ExpressionStatement >( ExpressionStatement{
							std::move( expressionResult->node )
						} )
					};
				}
			}
		}

		return {};
	}

	static AstResult< Statement > getStatement( std::vector< Token >::iterator current ) {
		if( AstResult< ExpressionStatement > expressionStatementResult = getExpressionStatement( current ) ) {
			return GeneratedAstNode< Statement >{
				expressionStatementResult->nextIterator,
				std::make_unique< Statement >( Statement{
					std::move( expressionStatementResult->node )
				} )
			};
		}

		return {};
	}

	Result< Program > getProgram( std::vector< Token > tokens ) {
		end = tokens.end();

		// Just a test for now
		try {
			Program program;

			if( AstResult< Statement > statementResult = getStatement( tokens.begin() ) ) {
				program.statements.emplace_back( std::move( statementResult->node ) );
			}

			return std::move( program );
		} catch( std::runtime_error e ) {
			return e.what();
		}
	}
}