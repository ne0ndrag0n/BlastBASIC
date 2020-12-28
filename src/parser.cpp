#include "parser.hpp"
#include "token.hpp"
#include "variant_visitor.hpp"
#include "ast.hpp"
#include "error.hpp"
#include <exception>
#include <optional>
#include <queue>

namespace GoldScorpion {

	// File-scope vars
	static std::vector< Token >::iterator end;

	// Forward declarations
	static AstResult< Expression > getExpression( std::vector< Token >::iterator current );
	static AstResult< Declaration > getDeclaration( std::vector< Token >::iterator current );
	// End forward declarations

	// Do not read iterator if it is past the end
	static std::optional< Token > readToken( std::vector< Token >::iterator iterator ) {
		if( iterator != end ) {
			return *iterator;
		}

		return {};
	}

	static std::vector< Token >::iterator expect( TokenType tokenType, std::vector< Token >::iterator iterator, const std::string& throwMessage ) {
		auto result = readToken( iterator );
		if( result && result->type == tokenType ) {
			return ++iterator;
		}

		Error{ throwMessage, readToken( iterator ) }.throwException();
		// as good as dead code
		throw std::runtime_error( "Internal compiler error" );
	}

	static std::optional< std::vector< Token >::iterator > attempt( TokenType tokenType, std::vector< Token >::iterator iterator ) {
		auto result = readToken( iterator );
		if( result && result->type == tokenType ) {
			return ++iterator;
		}

		return {};
	}

	static bool isNativeType( const Token& token ) {
		return (
			token.type == TokenType::TOKEN_U8 ||
			token.type == TokenType::TOKEN_U16 ||
			token.type == TokenType::TOKEN_U32 ||
			token.type == TokenType::TOKEN_S8 ||
			token.type == TokenType::TOKEN_S16 ||
			token.type == TokenType::TOKEN_S32 ||
			token.type == TokenType::TOKEN_STRING
		);
	}

	static bool isType( const Token& token ) {
		return isNativeType( token ) || token.type == TokenType::TOKEN_IDENTIFIER;
	}

	struct ParameterReturn {
		Parameter parameter;
		std::vector< Token >::iterator nextIterator;
	};
	static std::optional< ParameterReturn > getParameter( std::vector< Token >::iterator current ) {
		// A parameter takes the form IDENTIFIER "as" IDENTIFIER (of type form)
		auto nameResult = readToken( current );
		if( nameResult && nameResult->type == TokenType::TOKEN_IDENTIFIER ) {
			++current;
			auto asResult = readToken( current );
			if( asResult && asResult->type == TokenType::TOKEN_AS ) {
				++current;
				auto typeResult = readToken( current );
				if( typeResult && isType( *typeResult ) ) {
					++current;

					// Check if array type
					std::optional< Token > arraySize;
					auto leftBracketResult = readToken( current );
					if( leftBracketResult && leftBracketResult->type == TokenType::TOKEN_LEFT_BRACKET ) {
						++current;

						// Must be a number or identifier here
						auto numericResult = readToken( current );
						if( numericResult && ( numericResult->type == TokenType::TOKEN_IDENTIFIER || numericResult->type == TokenType::TOKEN_LITERAL_INTEGER ) ) {
							++current;
							arraySize = *numericResult;

							// Must be a closing bracket now
							if( readToken( current ) && current->type == TokenType::TOKEN_RIGHT_BRACKET ) {
								++current;
							} else {
								// Current will be incremented in the last step
								Error{ "Expected: closing \"]\" following an array size", readToken( current ) }.throwException();
							}
						} else {
							Error{ "Expected: integer or identifier following \"[\"", readToken( current ) }.throwException();
						}
					}

					return ParameterReturn{
						Parameter{ *nameResult, DataType{ *typeResult, arraySize } },
						current
					};
				}
			}
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
							std::make_unique< Primary >( Primary { currentToken } ),
							{}
						} )
					};
				case TokenType::TOKEN_LEFT_PAREN: {
					// Attempt to get expression
					AstResult< Expression > expression = getExpression( ++current );
					if( expression ) {
						// VariantResult valid if there is a closing paren in the next iterator
						if( expression->nextIterator->type == TokenType::TOKEN_RIGHT_PAREN ) {
							// Eat the current param and return the expression wrapped in a primary
							return GeneratedAstNode< Expression >{
								++expression->nextIterator,
								std::make_unique< Expression >( Expression {
									std::make_unique< Primary >( Primary {
										std::move( expression->node )
									} ),
									{}
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
								std::unique_ptr< Expression > expression = std::make_unique< Expression >( Expression {
									std::make_unique< BinaryExpression >( BinaryExpression {

										std::make_unique< Expression >( Expression {
											std::make_unique< Primary >( Primary{
												Token{ TokenType::TOKEN_SUPER, {}, 0, 0 }
											} ),
											{}
										} ),

										std::make_unique< Primary >( Primary {
											Token{ TokenType::TOKEN_DOT, {}, 0, 0 }
										} ),

										std::make_unique< Expression >( Expression {
											std::make_unique< Primary >( Primary{
												currentToken
											} ),
											{}
										} )

									} ),
									{}
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
						while( readToken( current ) && current->type == TokenType::TOKEN_COMMA ) {
							current++;

							if( AstResult< Expression > expression = getExpression( current ) ) {
								current = expression->nextIterator;
								arguments.emplace_back( std::move( expression->node ) );
							} else {
								// Error if an expression doesn't follow a comma
								Error{ "Expected: Expression following a \",\"", readToken( current ) }.throwException();
							}
						}
					}

					// There better be a right paren to close
					if( readToken( current ) && current->type == TokenType::TOKEN_RIGHT_PAREN ) {
						// Eat current
						current++;

						// Assemble CallExpression from current list of arguments
						queue.emplace( std::make_unique< Expression >( Expression {
							std::make_unique< CallExpression >( CallExpression{
								nullptr,
								std::move( arguments )
							} ),
							{}
						} ) );
					} else {
						Error{ "Expected: closing \")\"", readToken( current ) }.throwException();
					}

				} else if( readToken( current ) && current->type == TokenType::TOKEN_DOT ) {
					// Eat current
					current++;

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
									Error{ "Expected: Token of IDENTIFIER type", readToken( current ) }.throwException();
								}
							} else {
								Error{ "Expected: Primary of Token type", readToken( current ) }.throwException();
							}
						} else {
							Error{ "Expected: Expression of Primary type", readToken( current ) }.throwException();
						}
					} else {
						Error{ "Expected: Primary following \".\"", readToken( current ) }.throwException();
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
						std::move( *result ),
						{}
					} );
				} else if( auto result = std::get_if< std::unique_ptr< Primary > >( &queue.front()->value ) ) {
					std::unique_ptr< BinaryExpression > binary = std::make_unique< BinaryExpression >( BinaryExpression {
						std::move( primary->node ),

						std::make_unique< Primary >( Primary {
							Token{ TokenType::TOKEN_DOT, {}, 0, 0 }
						} ),

						std::make_unique< Expression >( Expression {
							std::move( *result ),
							{}
						} )
					} );

					primary->node = std::make_unique< Expression >( Expression {
						std::move( binary ),
						{}
					} );
				} else {
					Error{ "Internal compiler error (unexpected item in call-expression queue)", readToken( current ) }.throwException();
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
					std::make_unique< Expression >( Expression {
						std::make_unique< UnaryExpression >( UnaryExpression {
							std::make_unique< Primary >( Primary{ operatorToken } ),

							std::move( unary->node )
						} ),
						{}
					} )
				};
			} else {
				Error{ "Expected: terminal Expression following unary operator", readToken( current ) }.throwException();
			}
		} else {
			return getCall( current );
		}

		return {};
	}

	static AstResult< Expression > getFactor( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getUnary( current );
		if( result ) {
			// Keep taking unary expressions as long as the next token is a "/" or "*"
			current = result->nextIterator;
			while( readToken( current ) &&
				( current->type == TokenType::TOKEN_ASTERISK ||
				  current->type == TokenType::TOKEN_FORWARD_SLASH ||
				  current->type == TokenType::TOKEN_MODULO
				) ) {
				Token op = *current;

				AstResult< Expression > next = getUnary( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal Unary following operator \"*\" or \"/\"", readToken( current ) }.throwException();
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
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal Factor following operator \"-\" or \"+\"", readToken( current ) }.throwException();
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getBitwise( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getTerm( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) &&
				 ( current->type == TokenType::TOKEN_SHIFT_LEFT ||
				   current->type == TokenType::TOKEN_SHIFT_RIGHT ) ) {
				Token op = *current;

				AstResult< Expression > next = getTerm( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal Term following operator \">>\" or \"<<\"", readToken( current ) }.throwException();
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getComparison( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getBitwise( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) &&
				 ( current->type == TokenType::TOKEN_GREATER_THAN ||
				   current->type == TokenType::TOKEN_GREATER_THAN_EQUAL ||
				   current->type == TokenType::TOKEN_LESS_THAN ||
				   current->type == TokenType::TOKEN_LESS_THAN_EQUAL ) ) {
				Token op = *current;

				AstResult< Expression > next = getBitwise( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal Bitwise following operator \">\", \">=\", \"<\", or \"<=\"", readToken( current ) }.throwException();
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
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal Comparison following operator \"!=\" or \"==\"", readToken( current ) }.throwException();
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getBwAnd( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getEquality( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && current->type == TokenType::TOKEN_AMPERSAND ) {
				Token op = *current;

				AstResult< Expression > next = getEquality( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal Equality following operator \"&\"", readToken( current ) }.throwException();
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getBwXor( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getBwAnd( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && current->type == TokenType::TOKEN_CARET ) {
				Token op = *current;

				AstResult< Expression > next = getBwAnd( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal BwAnd following operator \"^\"", readToken( current ) }.throwException();
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getBwOr( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getBwXor( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && current->type == TokenType::TOKEN_PIPE ) {
				Token op = *current;

				AstResult< Expression > next = getBwXor( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal BwXor following operator \"|\"", readToken( current ) }.throwException();
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getLogicAnd( std::vector< Token >::iterator current ) {
		AstResult< Expression > result = getBwOr( current );
		if( result ) {
			current = result->nextIterator;
			while( readToken( current ) && current->type == TokenType::TOKEN_AND ) {
				Token op = *current;

				AstResult< Expression > next = getBwOr( ++current );
				if( next ) {
					current = next->nextIterator;

					// Form BinaryExpression
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal BwOr following operator \"and\"", readToken( current ) }.throwException();
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
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal LogicAnd following operator \"xor\"", readToken( current ) }.throwException();
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
					std::unique_ptr< Expression > binaryExpression = std::make_unique< Expression >( Expression {
						std::make_unique< BinaryExpression >( BinaryExpression {
							std::move( result->node ),

							std::make_unique< Primary >( Primary{ op } ),

							std::move( next->node )
						} ),
						{}
					} );

					result->node = std::move( binaryExpression );
				} else {
					Error{ "Expected: terminal LogicXor following operator \"or\"", readToken( current ) }.throwException();
				}
			}

			result->nextIterator = current;
		}

		return result;
	}

	static AstResult< Expression > getAssignment( std::vector< Token >::iterator current ) {
		// An assignment is a BinaryExpression with = as an operator
		// If we can parse two expressions split by an equals, this is an assignment expression
		// Otherwise - just skip to getLogicOr

		AstResult< Expression > lhs = getLogicOr( current );
		if( lhs ) {
			if( auto nextToken = readToken( lhs->nextIterator ) ) {
				if( nextToken->type == TokenType::TOKEN_EQUALS ) {
					AstResult< Expression > rhs = getAssignment( ++lhs->nextIterator );
					if( rhs ) {
						// Everything we need
						return GeneratedAstNode< Expression >{
							rhs->nextIterator,
							std::make_unique< Expression >( Expression {
								std::make_unique< AssignmentExpression >( AssignmentExpression{
									std::move( lhs->node ),

									std::move( rhs->node )
								} ),
								{}
							} )
						};
					} else {
						// If you specify an equals then there must be a successive expression
						Error{ "Expected: Expression following \"=\" token", readToken( current ) }.throwException();
					}
				}
			}
		}

		// If we get here then we want to fall through to a LogicOr expression
		return lhs;
	}

	static AstResult< Expression > getExpression( std::vector< Token >::iterator current ) {
		std::optional< Token > nearest = readToken( current );

		AstResult< Expression > result = getAssignment( current );

		if( result ) {
			result->node->nearestToken = nearest;
		}

		return result;
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

	static AstResult< ForStatement > getForStatement( std::vector< Token >::iterator current ) {
		if( readToken( current ) && current->type == TokenType::TOKEN_FOR ) {
			++current;

			auto indexResult = readToken( current );
			if( indexResult && indexResult->type == TokenType::TOKEN_IDENTIFIER ) {
				++current;

				if( readToken( current ) && current->type == TokenType::TOKEN_EQUALS ) {
					++current;

					if( AstResult< Expression > fromExpression = getExpression( current ) ) {
						current = fromExpression->nextIterator;

						if( readToken( current ) && current->type == TokenType::TOKEN_TO ) {
							++current;

							if( AstResult< Expression > toExpression = getExpression( current ) ) {
								current = toExpression->nextIterator;

								// optional "every" token
								std::optional< std::unique_ptr< Expression > > every;
								if( readToken( current ) && current->type == TokenType::TOKEN_EVERY ) {
									++current;

									if( AstResult< Expression > everyExpression = getExpression( current ) ) {
										current = everyExpression->nextIterator;
										every = std::move( everyExpression->node );
									} else {
										Error{ "Expected: expression following \"every\" token", readToken( current ) }.throwException();
									}
								}

								if( readToken( current ) && current->type == TokenType::TOKEN_NEWLINE ) {
									++current;

									std::vector< std::unique_ptr< Declaration > > body;
									while( AstResult< Declaration > declaration = getDeclaration( current ) ) {
										current = declaration->nextIterator;
										body.emplace_back( std::move( declaration->node ) );
									}

									// Must contain closing end
									if( readToken( current ) && current->type == TokenType::TOKEN_END ) {
										return GeneratedAstNode< ForStatement >{
											++current,
											std::make_unique< ForStatement >( ForStatement{
												*indexResult,
												std::move( fromExpression->node ),
												std::move( toExpression->node ),
												std::move( every ),
												std::move( body )
											} )
										};
									} else {
										Error{ "Expected: \"end\" token following ForStatement", readToken( current ) }.throwException();
									}
								} else {
									Error{ "Expected: newline following ForStatement header", readToken( current ) }.throwException();
								}

							} else {
								Error{ "Expected: expression following \"to\" token", readToken( current ) }.throwException();
							}
						} else {
							Error{ "Expected: \"to\" following expression", readToken( current ) }.throwException();
						}
					} else {
						Error{ "Expected: expression following \"=\" token", readToken( current ) }.throwException();
					}
				} else {
					Error{ "Expected: \"=\" following identifier", readToken( current ) }.throwException();
				}
			} else {
				Error{ "Expected: identifier following \"for\" token", readToken( current ) }.throwException();
			}
		}

		return {};
	}

	static AstResult< IfStatement > getIfStatement( std::vector< Token >::iterator current ) {
		if( auto afterIf = attempt( TokenType::TOKEN_IF, current ) ) {
			current = *afterIf;

			std::vector< std::unique_ptr< Expression > > conditions;
			std::vector< std::vector< std::unique_ptr< Declaration > > > bodies;

			if( AstResult< Expression > baseCondition = getExpression( current ) ) {
				current = expect( TokenType::TOKEN_THEN, baseCondition->nextIterator, "Expected: \"then\" following if conditional" );
				conditions.emplace_back( std::move( baseCondition->node ) );

				// Zero or more declarations following "then"
				{
					std::vector< std::unique_ptr< Declaration > > body;
					while( AstResult< Declaration > declaration = getDeclaration( current ) ) {
						current = declaration->nextIterator;
						body.emplace_back( std::move( declaration->node ) );
					}
					bodies.emplace_back( std::move( body ) );
				}

				// After that, there are zero or more elseif statements
				while( true ) {
					if( auto afterNextElse = attempt( TokenType::TOKEN_ELSE, current ) ) {
						if( auto afterNextIf = attempt( TokenType::TOKEN_IF, *afterNextElse ) ) {
							current = *afterNextIf;

							if( AstResult< Expression > elifCondition = getExpression( current ) ) {
								current = expect( TokenType::TOKEN_THEN, elifCondition->nextIterator, "Expected: \"then\" following else if expression" );
								conditions.emplace_back( std::move( elifCondition->node ) );

								// Zero or more declarations following "then"
								std::vector< std::unique_ptr< Declaration > > body;
								while( AstResult< Declaration > declaration = getDeclaration( current ) ) {
									current = declaration->nextIterator;
									body.emplace_back( std::move( declaration->node ) );
								}
								bodies.emplace_back( std::move( body ) );

								continue;
							} else {
								Error{ "Expected: Expression following \"else\" \"if\" sequence", readToken( current ) }.throwException();
							}
						}
					}

					break;
				}

				// One or none "else" statements
				if( auto afterElse = attempt( TokenType::TOKEN_ELSE, current ) ) {
					current = *afterElse;

					std::vector< std::unique_ptr< Declaration > > body;
					while( AstResult< Declaration > declaration = getDeclaration( current ) ) {
						current = declaration->nextIterator;
						body.emplace_back( std::move( declaration->node ) );
					}
					bodies.emplace_back( std::move( body ) );
				}

				// Finally a closing end
				return GeneratedAstNode< IfStatement >{
					expect( TokenType::TOKEN_END, current, "Expected: \"end\" token following IfStatement" ),
					std::make_unique< IfStatement >( IfStatement{ std::move( conditions ), std::move( bodies ) } )
				};
			} else {
				Error{ "Expected: Expression following \"if\"", readToken( current ) }.throwException();
			}
		}

		return {};
	}

	static AstResult< ReturnStatement > getReturnStatement( std::vector< Token >::iterator current ) {
		auto afterReturn = attempt( TokenType::TOKEN_RETURN, current );
		if( afterReturn ) {
			current = *afterReturn;

			std::optional< std::unique_ptr< Expression > > returnExpression;
			if( AstResult< Expression > expression = getExpression( current ) ) {
				current = expression->nextIterator;
				returnExpression = std::move( expression->node );
			}

			return GeneratedAstNode< ReturnStatement >{
				expect( TokenType::TOKEN_NEWLINE, current, "Expected: newline after ReturnStatement" ),
				std::make_unique< ReturnStatement >( ReturnStatement{
					std::move( returnExpression )
				} )
			};
		}

		return {};
	}

	static AstResult< AsmStatement > getAsmStatement( std::vector< Token >::iterator current ) {
		auto afterAsm = attempt( TokenType::TOKEN_ASM, current );
		if( afterAsm ) {
			current = *afterAsm;

			auto potentialText = readToken( current );
			if( potentialText && potentialText->type == TokenType::TOKEN_TEXT ) {
				current = expect( TokenType::TOKEN_END, ++current, "Expected: \"end\" token following AsmStatement body" );

				return GeneratedAstNode< AsmStatement >{
					expect( TokenType::TOKEN_NEWLINE, current, "Expected: newline following AsmStatement" ),
					std::make_unique< AsmStatement >( AsmStatement { *potentialText } )
				};
			} else {
				Error{ "Expected: inline asm body following \"asm\" token", readToken( current ) }.throwException();
			}
		}

		return {};
	}

	static AstResult< WhileStatement > getWhileStatement( std::vector< Token >::iterator current ) {
		auto afterWhile = attempt( TokenType::TOKEN_WHILE, current );
		if( afterWhile ) {
			current = *afterWhile;

			if( AstResult< Expression > expression = getExpression( current ) ) {
				current = expect( TokenType::TOKEN_NEWLINE, expression->nextIterator, "Expected: newline following Expression" );

				std::vector< std::unique_ptr< Declaration > > body;
				while( AstResult< Declaration > declaration = getDeclaration( current ) ) {
					current = declaration->nextIterator;
					body.emplace_back( std::move( declaration->node ) );
				}

				return GeneratedAstNode< WhileStatement >{
					expect( TokenType::TOKEN_END, current, "Expected: \"end\" token following WhileStatement body" ),
					std::make_unique< WhileStatement >( WhileStatement{ std::move( expression->node ), std::move( body ) } )
				};
			} else {
				Error{ "Expected: Expression following \"while\" token", readToken( current ) }.throwException();
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

		if( AstResult< ForStatement > forStatementResult = getForStatement( current ) ) {
			return GeneratedAstNode< Statement >{
				forStatementResult->nextIterator,
				std::make_unique< Statement >( Statement {
					std::move( forStatementResult->node )
				} )
			};
		}

		if( AstResult< IfStatement > ifStatementResult = getIfStatement( current ) ) {
			return GeneratedAstNode< Statement >{
				ifStatementResult->nextIterator,
				std::make_unique< Statement >( Statement {
					std::move( ifStatementResult->node )
				} )
			};
		}

		if( AstResult< ReturnStatement > returnStatementResult = getReturnStatement( current ) ) {
			return GeneratedAstNode< Statement >{
				returnStatementResult->nextIterator,
				std::make_unique< Statement >( Statement {
					std::move( returnStatementResult->node )
				} )
			};
		}

		if( AstResult< AsmStatement > asmStatementResult = getAsmStatement( current ) ) {
			return GeneratedAstNode< Statement >{
				asmStatementResult->nextIterator,
				std::make_unique< Statement >( Statement {
					std::move( asmStatementResult->node )
				} )
			};
		}

		if( AstResult< WhileStatement > whileStatementResult = getWhileStatement( current ) ) {
			return GeneratedAstNode< Statement >{
				whileStatementResult->nextIterator,
				std::make_unique< Statement >( Statement {
					std::move( whileStatementResult->node )
				} )
			};
		}

		return {};
	}

	static AstResult< FunctionDeclaration > getFunctionDeclaration( std::vector< Token >::iterator current ) {
		auto functionResult = readToken( current );
		if( functionResult && functionResult->type == TokenType::TOKEN_FUNCTION ) {
			++current;

			// Optional identifier
			std::optional< Token > name;
			auto nameResult = readToken( current );
			if( nameResult && nameResult->type == TokenType::TOKEN_IDENTIFIER ) {
				++current;
				name = *nameResult;
			}

			// Eat ( token
			auto leftParenResult = readToken( current );
			if( leftParenResult && leftParenResult->type == TokenType::TOKEN_LEFT_PAREN ) {
				++current;
			} else {
				Error{ "Expected: \"(\" token following function or function identifier", readToken( current ) }.throwException();
			}

			// Optional parameters
			std::vector< Parameter > arguments;
			if( auto firstArgumentResult = getParameter( current ) ) {
				current = firstArgumentResult->nextIterator;
				arguments.push_back( firstArgumentResult->parameter );

				while( readToken( current ) && current->type == TokenType::TOKEN_COMMA ) {
					++current;

					if( auto nextArgumentResult = getParameter( current ) ) {
						current = nextArgumentResult->nextIterator;
						arguments.push_back( nextArgumentResult->parameter );
					} else {
						Error{ "Expected: parameter following \",\" token", readToken( current ) }.throwException();
					}
				}
			}

			// Eat ) token
			auto rightParenResult = readToken( current );
			if( rightParenResult && rightParenResult->type == TokenType::TOKEN_RIGHT_PAREN ) {
				++current;
			} else {
				Error{ "Expected: \")\" token following function argument list", readToken( current ) }.throwException();
			}

			// Optional "as" return type
			std::optional< Token > returnType;
			auto asResult = readToken( current );
			if( asResult && asResult->type == TokenType::TOKEN_AS ) {
				++current;

				// Now expect identifier of type form
				auto returnTypeResult = readToken( current );
				if( returnTypeResult && isType( *returnTypeResult ) ) {
					++current;
					returnType = *returnTypeResult;
				} else {
					Error{ "Expected: identifier following \"as\" token", readToken( current ) }.throwException();
				}
			}

			// Now begins a completely optional list of declarations
			std::vector< std::unique_ptr< Declaration > > body;
			while( AstResult< Declaration > declaration = getDeclaration( current ) ) {
				body.emplace_back( std::move( declaration->node ) );
				current = declaration->nextIterator;
			}

			// End must close function declaration
			auto endResult = readToken( current );
			if( endResult && endResult->type == TokenType::TOKEN_END ) {
				return GeneratedAstNode< FunctionDeclaration >{
					++current,
					std::make_unique< FunctionDeclaration >( FunctionDeclaration{
						name,
						arguments,
						returnType,
						std::move( body )
					} )
				};
			} else {
				Error{ "Expected: \"end\" token following function body", readToken( current ) }.throwException();
			}
		}

		return {};
	}

	static AstResult< TypeDeclaration > getTypeDeclaration( std::vector< Token >::iterator current ) {
		auto typeResult = readToken( current );
		if( typeResult && typeResult->type == TokenType::TOKEN_TYPE ) {
			++current;

			auto nameResult = readToken( current );
			if( nameResult && nameResult->type == TokenType::TOKEN_IDENTIFIER ) {
				++current;

				auto newlineResult = readToken( current );
				if( newlineResult && newlineResult->type == TokenType::TOKEN_NEWLINE ) {
					++current;

					// Burn any newlines between here and the first parameter
					while( readToken( current ) && current->type == TokenType::TOKEN_NEWLINE ) {
						current++;
					}

					// Must be at least one parameter
					std::vector< Parameter > fields;
					// Keep eating parameters and burning newlines as long as we can
					while( auto paramResult = getParameter( current ) ) {
						current = paramResult->nextIterator;
						fields.push_back( paramResult->parameter );

						while( readToken( current ) && current->type == TokenType::TOKEN_NEWLINE ) {
							current++;
						}
					}

					if( fields.empty() ) {
						Error{ "Expected: at least one field in TypeDeclaration", readToken( current ) }.throwException();
					}

					// Zero or more functions
					std::vector< std::unique_ptr< FunctionDeclaration > > functions;
					while( AstResult< FunctionDeclaration > function = getFunctionDeclaration( current ) ) {
						current = function->nextIterator;
						functions.emplace_back( std::move( function->node ) );

						while( readToken( current ) && current->type == TokenType::TOKEN_NEWLINE ) {
							current++;
						}
					}

					// Must have a closing end
					auto endResult = readToken( current );
					if( endResult && endResult->type == TokenType::TOKEN_END ) {
						return GeneratedAstNode< TypeDeclaration >{
							++current,
							std::make_unique< TypeDeclaration >( TypeDeclaration{
								*nameResult,
								fields,
								std::move( functions )
							} )
						};
					} else {
						Error{ "Expected: \"end\" token following TypeDeclaration", readToken( current ) }.throwException();
					}
				} else {
					Error{ "Expected: newline following identifier", readToken( current ) }.throwException();
				}
			} else {
				Error{ "Expected: identifier following \"type\" token", readToken( current ) }.throwException();
			}
		}

		return {};
	}

	static AstResult< VarDeclaration > getVarDeclaration( std::vector< Token >::iterator current ) {
		auto defResult = readToken( current );
		if( defResult && defResult->type == TokenType::TOKEN_DEF ) {
			++current;

			// identifier AS type
			auto parameterResult = getParameter( current );
			if( parameterResult ) {
				current = parameterResult->nextIterator;

				// Optional: Equals to define a default value
				std::optional< std::unique_ptr< Expression > > assignment;
				auto equalsResult = readToken( current );
				if( equalsResult && equalsResult->type == TokenType::TOKEN_EQUALS ) {
					++current;

					if( AstResult< Expression > expression = getExpression( current ) ) {
						current = expression->nextIterator;
						assignment = std::move( expression->node );
					} else {
						Error{ "Expected: Expression following \"=\" token", readToken( current ) }.throwException();
					}
				}

				// Newline at the end!
				auto newlineResult = readToken( current );
				if( newlineResult && newlineResult->type == TokenType::TOKEN_NEWLINE ) {
					// Return result
					return GeneratedAstNode< VarDeclaration >{
						++current,
						std::make_unique< VarDeclaration >( VarDeclaration{
							parameterResult->parameter,
							std::move( assignment )
						} )
					};
				} else {
					Error{ "Expected: newline following VarDeclaration", readToken( current ) }.throwException();
				}
			} else {
				Error{ "Expected: parameter following \"def\" token", readToken( current ) }.throwException();
			}
		}

		return {};
	}

	static AstResult< ConstDeclaration > getConstDeclaration( std::vector< Token >::iterator current ) {
		auto afterConst = attempt( TokenType::TOKEN_CONST, current );
		if( afterConst ) {
			current = *afterConst;

			auto parameter = getParameter( current );
			if( parameter ) {
				current = expect( TokenType::TOKEN_EQUALS, parameter->nextIterator, "Expected: \"=\" token following parameter" );

				if( AstResult< Expression > expression = getExpression( current ) ) {
					return GeneratedAstNode< ConstDeclaration >{
						expect( TokenType::TOKEN_NEWLINE, expression->nextIterator, "Expected: newline following ConstDeclaration" ),
						std::make_unique< ConstDeclaration >( ConstDeclaration {
							parameter->parameter,
							std::move( expression->node )
						} )
					};
				} else {
					Error{ "Expected: Expression following \"=\" statement", readToken( current ) }.throwException();
				}
			} else {
				Error{ "Expected: parameter after \"const\" token", readToken( current ) }.throwException();
			}
		}

		return {};
	}

	static AstResult< ImportDeclaration > getImportDeclaration( std::vector< Token >::iterator current ) {
		auto tokenResult = readToken( current );
		if( tokenResult && tokenResult->type == TokenType::TOKEN_IMPORT ) {
			// Get a literal string + \n or it's a compiler error
			++current;
			auto literalStringResult = readToken( current );
			if( literalStringResult && literalStringResult->type == TokenType::TOKEN_TEXT ) {
				++current;

				auto newlineResult = readToken( current );
				if( newlineResult && newlineResult->type == TokenType::TOKEN_NEWLINE ) {
					return GeneratedAstNode< ImportDeclaration >{
						++current,
						std::make_unique< ImportDeclaration >( ImportDeclaration{
							std::get< std::string >( *literalStringResult->value )
						} )
					};
				}
			}
		}

		return {};
	}

	static AstResult< Annotation > getAnnotation( std::vector< Token >::iterator current ) {
		auto afterAt = attempt( TokenType::TOKEN_AT_SYMBOL, current );
		if( afterAt ) {
			current = expect( TokenType::TOKEN_LEFT_BRACKET, *afterAt, "Expected: \"[\" after annotation symbol" );

			std::vector< std::unique_ptr< Expression > > directives;

			// At least one expression
			if( AstResult< Expression > expression = getExpression( current ) ) {
				current = expression->nextIterator;
				directives.emplace_back( std::move( expression->node ) );
			} else {
				Error{ "Expected: expression following annotation declaration", readToken( current ) }.throwException();
			}

			// Zero or more additional expressions each following a comma
			while( auto afterComma = attempt( TokenType::TOKEN_COMMA, current ) ) {
				current = *afterComma;

				if( AstResult< Expression > expression = getExpression( current ) ) {
					current = expression->nextIterator;
					directives.emplace_back( std::move( expression->node ) );
				} else {
					Error{ "Expected: expression following \",\" token", readToken( current ) }.throwException();
				}
			}

			current = expect( TokenType::TOKEN_RIGHT_BRACKET, current, "Expected: \"]\" following expression list" );

			return GeneratedAstNode< Annotation >{
				expect( TokenType::TOKEN_NEWLINE, current, "Expected: newline following annotation declaration" ),
				std::make_unique< Annotation >( Annotation { std::move( directives ) } )
			};
		}

		return {};
	}

	static AstResult< Declaration > getDeclaration( std::vector< Token >::iterator current ) {
		// Burn newlines before
		while( readToken( current ) && current->type == TokenType::TOKEN_NEWLINE ) {
			current++;
		}

		AstResult< Declaration > result = {};

		// Must return one of: annotation, typeDecl, funDecl, varDecl, constDecl, importDecl, statement
		if( auto annotation = getAnnotation( current ) ) {
			current = annotation->nextIterator;

			result = GeneratedAstNode< Declaration >{
				current,
				std::make_unique< Declaration >( Declaration{
					std::move( annotation->node )
				})
			};
		} else if( auto typeDecl = getTypeDeclaration( current ) ) {
			current = typeDecl->nextIterator;

			result = GeneratedAstNode< Declaration >{
				current,
				std::make_unique< Declaration >( Declaration{
					std::move( typeDecl->node )
				} )
			};
		} else if( auto funDecl = getFunctionDeclaration( current ) ) {
			current = funDecl->nextIterator;

			result = GeneratedAstNode< Declaration > {
				current,
				std::make_unique< Declaration >( Declaration {
					std::move( funDecl->node )
				} )
			};
		} else if( auto varDecl = getVarDeclaration( current ) ) {
			current = varDecl->nextIterator;

			result = GeneratedAstNode< Declaration > {
				current,
				std::make_unique< Declaration >( Declaration {
					std::move( varDecl->node )
				} )
			};
		} else if( auto constDecl = getConstDeclaration( current ) ) {
			current = constDecl->nextIterator;

			result = GeneratedAstNode< Declaration > {
				current,
				std::make_unique< Declaration >( Declaration {
					std::move( constDecl->node )
				} )
			};
		} else if( auto importDecl = getImportDeclaration( current ) ) {
			current = importDecl->nextIterator;

			result = GeneratedAstNode< Declaration > {
				current,
				std::make_unique< Declaration >( Declaration {
					std::move( importDecl->node )
				} )
			};
		} else if( auto statement = getStatement( current ) ) {
			current = statement->nextIterator;

			result = GeneratedAstNode< Declaration >{
				current,
				std::make_unique< Declaration >( Declaration {
					std::move( statement->node )
				} )
			};
		}

		// Burn newlines after
		while( readToken( current ) && current->type == TokenType::TOKEN_NEWLINE ) {
			current++;
		}

		return result;
	}

	VariantResult< Program > getProgram( std::vector< Token > tokens ) {
		end = tokens.end();

		// Just a test for now
		try {
			Program program;

			std::vector< Token >::iterator current = tokens.begin();
			while( current != tokens.end() || current->type != TokenType::TOKEN_NONE ) {
				if( AstResult< Declaration > declaration = getDeclaration( current ) ) {
					program.statements.emplace_back( std::move( declaration->node ) );
					current = declaration->nextIterator;
				} else {
					break;
				}
			}

			return std::move( program );
		} catch( std::runtime_error e ) {
			return e.what();
		}
	}
}