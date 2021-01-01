#include "generator.hpp"
#include "variant_visitor.hpp"
#include "memory_tracker.hpp"
#include "arch/m68k/instruction.hpp"
#include "type_tools.hpp"
#include "token.hpp"
#include "error.hpp"
#include <cstdio>
#include <exception>
#include <optional>
#include <unordered_map>

namespace GoldScorpion {

	enum class ExpressionTypeTag { INVALID, U8, U16, U32, S8, S16, S32, STRING, UDT };
	struct ExpressionDataType {
		ExpressionTypeTag type;
		std::optional< UserDefinedType > udt;
	};

	static ExpressionDataType getType( const Expression& node, Assembly& assembly );
	static ExpressionDataType getType( const Primary& node, Assembly& assembly );
	static void generate( const Expression& node, Assembly& assembly );

	static const std::unordered_map< std::string, ExpressionTypeTag > types = {
		{ "u8", ExpressionTypeTag::U8 },
		{ "u16", ExpressionTypeTag::U16 },
		{ "u32", ExpressionTypeTag::U32 },
		{ "s8", ExpressionTypeTag::S8 },
		{ "s16", ExpressionTypeTag::S16 },
		{ "s32", ExpressionTypeTag::S32 },
		{ "string", ExpressionTypeTag::STRING }
	};

	static char getTypeComparison( ExpressionDataType type ) {
		switch( type.type ) {
			case ExpressionTypeTag::INVALID:
			default:
				return 0;
			case ExpressionTypeTag::U8:
			case ExpressionTypeTag::S8:
				return 1;
			case ExpressionTypeTag::U16:
			case ExpressionTypeTag::S16:
				return 2;
			case ExpressionTypeTag::U32:
			case ExpressionTypeTag::S32:
			case ExpressionTypeTag::STRING:
				return 3;
		}
	}

	static bool isSigned( ExpressionDataType type ) {
		switch( type.type ) {
			case ExpressionTypeTag::S8:
			case ExpressionTypeTag::S16:
			case ExpressionTypeTag::S32:
				return true;
			default:
				return false;
		}
	}

	static bool isOneSigned( ExpressionDataType a, ExpressionDataType b ) {
		return ( isSigned( a ) && !isSigned( b ) ) ||
			( !isSigned( a ) && isSigned( b ) );
	}

	static ExpressionDataType scrubSigned( ExpressionDataType type ) {
		switch( type.type ) {
			case ExpressionTypeTag::S8:
				return ExpressionDataType{ ExpressionTypeTag::U8, {} };
			case ExpressionTypeTag::S16:
				return ExpressionDataType{ ExpressionTypeTag::U16, {} };
			case ExpressionTypeTag::S32:
				return ExpressionDataType{ ExpressionTypeTag::U32, {} };
			default:
				return type;
		}
	}

	static m68k::OperatorSize typeToWordSize( ExpressionDataType type ) {
		switch( type.type ) {
			case ExpressionTypeTag::U8:
			case ExpressionTypeTag::S8:
				return m68k::OperatorSize::BYTE;
			case ExpressionTypeTag::U16:
			case ExpressionTypeTag::S16:
				return m68k::OperatorSize::WORD;
			case ExpressionTypeTag::U32:
			case ExpressionTypeTag::S32:
			default:
				return m68k::OperatorSize::LONG;
		}
	}

	static m68k::InstructionMetadata typeToPointerMetadata( ExpressionDataType type ) {
		switch( type.type ) {
			case ExpressionTypeTag::U8:
			case ExpressionTypeTag::S8:
				return m68k::InstructionMetadata::POINTER_BYTE;
			case ExpressionTypeTag::U16:
			case ExpressionTypeTag::S16:
			default:
				return m68k::InstructionMetadata::POINTER_WORD;
			case ExpressionTypeTag::U32:
			case ExpressionTypeTag::S32:
				return m68k::InstructionMetadata::POINTER_LONG;
			case ExpressionTypeTag::STRING:
				return m68k::InstructionMetadata::POINTER_STRING;
		}
	}

	static m68k::OperatorSize getDereferenceSize( const std::set< m68k::InstructionMetadata >& instructionMetadata ) {
		if( instructionMetadata.count( m68k::InstructionMetadata::POINTER_BYTE ) ) {
			return m68k::OperatorSize::BYTE;
		} else if( instructionMetadata.count( m68k::InstructionMetadata::POINTER_WORD ) ) {
			return m68k::OperatorSize::WORD;
		} else {
			return m68k::OperatorSize::LONG;
		}
	}

	static long getMaskByType( m68k::OperatorSize size ) {
		switch( size ) {
			case m68k::OperatorSize::BYTE:
				return 0xFF;
			default:
			case m68k::OperatorSize::WORD:
				return 0xFFFF;
			case m68k::OperatorSize::LONG:
				return 0xFFFFFFFF;
		}
	}

	static ExpressionDataType getLiteralType( long literal ) {
		// Negative values mean a signed value is required
		if( literal < 0 ) {

			if( literal >= -127 ) {
				return ExpressionDataType{ ExpressionTypeTag::S8, {} };
			}

			if( literal >= -32767 ) {
				return ExpressionDataType{ ExpressionTypeTag::S16, {} };
			}

			return ExpressionDataType{ ExpressionTypeTag::S32, {} };

		} else {

			if( literal <= 255 ) {
				return ExpressionDataType{ ExpressionTypeTag::U8, {} };
			}

			if( literal <= 65535 ) {
				return ExpressionDataType{ ExpressionTypeTag::U16, {} };
			}

			return ExpressionDataType{ ExpressionTypeTag::U32, {} };
		}
	}

	static ExpressionDataType getIdentifierType( const std::string& typeId ) {
		auto it = types.find( typeId );
		if( it != types.end() ) {
			return ExpressionDataType{ it->second, {} };
		}

		return ExpressionDataType{ ExpressionTypeTag::INVALID, {} };
	}

	static long expectLong( const Token& token, const std::string& error ) {
		if( token.value ) {
			if( auto longValue = std::get_if< long >( &*( token.value ) ) ) {
				return *longValue;
			}
		}

		Error{ error, token }.throwException();
		return 0;
	}

	static std::string expectString( const Token& token, const std::string& error ) {
		if( token.value ) {
			if( auto stringValue = std::get_if< std::string >( &*( token.value ) ) ) {
				return *stringValue;
			}
		}

		Error{ error, token }.throwException();
		return "";
	}

	static std::optional< Token > attemptToken( const Primary& primary ) {
		if( auto token = std::get_if< Token >( &primary.value ) ) {
			return *token;
		}

		return {};
	}

	static Token expectToken( const Primary& primary, std::optional< Token > nearestToken, const std::string& error ) {
		if( auto token = attemptToken( primary ) ) {
			return *token;
		}

		Error{ error, nearestToken }.throwException();
		return Token{ TokenType::TOKEN_NONE, {}, 0, 0 };
	}

	// Used primarily for errors
	static std::optional< Token > getNearestToken( const Expression& expression ) {
		std::optional< Token > result;

		if( auto primaryResult = std::get_if< std::unique_ptr< Primary > >( &expression.value ) ) {
			const Primary& primary = **primaryResult;

			if( auto token = std::get_if< Token >( &primary.value ) ) {
				return *token;
			}
		}

		return result;
	}

	static ExpressionDataType getType( const Primary& node, Assembly& assembly ) {
		ExpressionDataType result;

		// We can directly determine the primary value for any Token variant
		// Otherwise, we need to go deeper
		std::visit( overloaded {
			[ &result, &assembly ]( const Token& token ) {
				// The only two types of token expected here are TOKEN_LITERAL_INTEGER, TOKEN_LITERAL_STRING, and TOKEN_IDENTIFIER
				// If TOKEN_IDENTIFIER is provided, the underlying type must not be of a custom type
				switch( token.type ) {
					case TokenType::TOKEN_LITERAL_INTEGER: {
						result = getLiteralType( expectLong( token, "Expected: long type for literal integer token" ) );
						break;
					}
					case TokenType::TOKEN_LITERAL_STRING: {
						result = ExpressionDataType{ ExpressionTypeTag::STRING, {} };
						break;
					}
					case TokenType::TOKEN_IDENTIFIER: {
						std::string id = expectString( token, "Internal compiler error" );
						auto memoryQuery = assembly.memory.find( id );
						if( !memoryQuery ) {
							Error{ std::string( "Undefined identifier: " ) + id, token }.throwException();
						}

						std::string typeId = unwrapTypeId( MemoryTracker::unwrapValue( *memoryQuery ).type );
						result = getIdentifierType( typeId );
						if( result.type == ExpressionTypeTag::INVALID ) {
							// Possibly a UDT
							std::optional< UserDefinedType > udt = assembly.memory.findUdt( typeId );
							if( udt ) {
								result.type = ExpressionTypeTag::UDT;
								result.udt = udt;
							}

							// Don't throw error here!
							// Invalid refs may be transformed into other kinds of refs after return
						}
						break;
					}
					default:
						Error{ "Expected: integer, string, or identifier as expression operand", token }.throwException();
				}
			},
			[ &result, &assembly ]( const std::unique_ptr< Expression >& expression ) {
				result = getType( *expression, assembly );
			}
		}, node.value );

		return result;
	}

	static ExpressionDataType getType( const BinaryExpression& node, Assembly& assembly ) {
		// The type of a BinaryExpression is the larger of the two children

		ExpressionDataType lhs = getType( *node.lhsValue, assembly );
		ExpressionDataType rhs = getType( *node.rhsValue, assembly );

		// If either lhs or rhs return an invalid comparison
		if( lhs.type == ExpressionTypeTag::INVALID || rhs.type == ExpressionTypeTag::INVALID ) {
			return ExpressionDataType{ ExpressionTypeTag::INVALID, {} };
		}

		// If lhs is a UDT then the right hand side of the expression is the expression type
		if( lhs.type == ExpressionTypeTag::UDT ) {
			// UDT LHS requires special consideration:
			// - Operator is dot - Return result is the field type on the UDT.
			// - Operator is non-dot - Return result is the same UDT (cannot apply operator to unlike UDTs)
			TokenType tokenOp = expectToken( *node.op, getNearestToken( *node.lhsValue ),  "Internal compiler error" ).type;
			if( tokenOp == TokenType::TOKEN_DOT ) {
				// Get UDT in left hand side
				// UDT assumed to be on LHS
				std::optional< Token > rhsToken = getNearestToken( *node.rhsValue );
				if( !rhsToken ) {
					Error{ "Internal compiler error", {} }.throwException();
				}

				std::optional< UdtField > udtField = assembly.memory.findUdtField( lhs.udt->id, expectString( *rhsToken, "Internal compiler error" ) );
				if( !udtField ) {
					Error{ "Internal compiler error", {} }.throwException();
				}

				ExpressionDataType rhsType = getIdentifierType( unwrapTypeId( udtField->type ) );
				if( rhsType.type == ExpressionTypeTag::INVALID ) {
					std::optional< UserDefinedType > udt = assembly.memory.findUdt( unwrapTypeId( udtField->type ) );
					if( udt ) {
						rhsType.type = ExpressionTypeTag::UDT;
						rhsType.udt = udt;
					}
				}

				return rhsType;
			}
		}

		// Otherwise the data type of the BinaryExpression is the larger of lhs, rhs
		if( getTypeComparison( rhs ) >= getTypeComparison( lhs ) ) {
			return isOneSigned( lhs, rhs ) ? scrubSigned( rhs ) : rhs;
		} else {
			return isOneSigned( lhs, rhs ) ? scrubSigned( lhs ) : lhs;
		}
	}

	static ExpressionDataType getType( const Expression& node, Assembly& assembly ) {

		if( auto binaryExpression = std::get_if< std::unique_ptr< BinaryExpression > >( &node.value ) ) {
 			return getType( **binaryExpression, assembly );
		}

		if( auto primaryExpression = std::get_if< std::unique_ptr< Primary > >( &node.value ) ) {
			return getType( **primaryExpression, assembly );
		}

		// Many node types not yet implemented
		return ExpressionDataType{ ExpressionTypeTag::INVALID, {} };
	}

	static void generate( const Primary& node, Assembly& assembly ) {
		ExpressionDataType primaryType = getType( node, assembly );

		std::visit( overloaded {
			[ primaryType, &assembly, &node ]( const Token& token ) {

				if( token.type == TokenType::TOKEN_LITERAL_INTEGER ) {
					// Generate immediate move instruction onto stack
					assembly.instructions.push_back(
						m68k::Instruction {
							m68k::Operator::MOVE,
							typeToWordSize( primaryType ),
							m68k::Operand { 0, m68k::OperandType::IMMEDIATE, 0, expectLong( token, "Internal compiler error" ) },
							m68k::Operand { 0, m68k::OperandType::REGISTER_d0, 0, 0 },
							{}
						}
					);
				} else if( token.type == TokenType::TOKEN_IDENTIFIER ) {
					// Push a literal version of the indirect value
					// Query memory for the value
					std::string id = expectString( token, "Internal compiler error" );
					auto memoryQuery = assembly.memory.find( id );
					if( memoryQuery ) {
						std::visit( overloaded {
							[ primaryType, &assembly ]( const GlobalMemoryElement& element ) {
								assembly.instructions.push_back(
									m68k::Instruction {
										m68k::Operator::MOVE,
										m68k::OperatorSize::LONG,
										m68k::Operand { 0, m68k::OperandType::IMMEDIATE, 0, element.offset },
										m68k::Operand { 0, m68k::OperandType::REGISTER_a0, 0, 0 },
										{ typeToPointerMetadata( primaryType ) }
									}
								);
							},
							[ primaryType, &assembly ]( const StackMemoryElement& element ) {
								// Must push a series of instructions:
								// 1. Move stack pointer into a0
								// 2. Add offset to a0
								// 3. Push a0 back onto the stack
								assembly.instructions.push_back(
									m68k::Instruction {
										m68k::Operator::MOVE,
										m68k::OperatorSize::LONG,
										m68k::Operand { 0, m68k::OperandType::REGISTER_sp, 0, 0 },
										m68k::Operand { 0, m68k::OperandType::REGISTER_a0, 0, 0 },
										{}
									}
								);

								assembly.instructions.push_back(
									m68k::Instruction {
										m68k::Operator::ADD,
										m68k::OperatorSize::LONG,
										m68k::Operand{ 0, m68k::OperandType::IMMEDIATE, 0, element.offset },
										m68k::Operand{ 0, m68k::OperandType::REGISTER_a0, 0, 0 },
										{ typeToPointerMetadata( primaryType ) }
									}
								);
							}
						}, *memoryQuery );
					} else {
						Error{ std::string( "Undefined identifier: " ) + id, token }.throwException();
					}
				} else {
					Error{ "Expected: TOKEN_LITERAL_INTEGER to generate Primary code", token }.throwException();
				}

			},
			[ &assembly ]( const std::unique_ptr< Expression >& expression ) {
				generate( *expression, assembly );
			}
		}, node.value );
	}

	static void generate( const BinaryExpression& node, Assembly& assembly ) {
		// Get type of left and right hand sides
		// The largest of the two types is used to generate code
		ExpressionDataType type = getType( node, assembly );
		m68k::OperatorSize wordSize = typeToWordSize( type );

		// Now, depending on the operator given, apply the RHS
		TokenType tokenOp = expectToken( *node.op, getNearestToken( *node.lhsValue ),  "Expected: Token as BinaryExpression operator" ).type;
		if( tokenOp == TokenType::TOKEN_DOT ) {
			// LHS is an address and RHS is a field name
			// If field type is another UDT, then push another address
			// Otherwise, push an actual value
		} else {
			// Expressions are evaluated right to left
			// All operations work on stack
			// Elision step will take care of redundant assembly
			ExpressionDataType lhsType = getType( *node.lhsValue, assembly );
			ExpressionDataType rhsType = getType( *node.rhsValue, assembly );

			generate( *node.rhsValue, assembly );
			// Push RHS for later - RHS may be contained in d0 or a0 depending on last instruction
			// This is determined by examining the last instruction generated
			bool rhsIsAddress = assembly.instructions.back().argument2->type == m68k::OperandType::REGISTER_a0;
			std::set< m68k::InstructionMetadata > rhsMetadata = assembly.instructions.back().metadata;
			assembly.instructions.push_back(
				m68k::Instruction {
					m68k::Operator::MOVE,
					assembly.instructions.back().size,
					m68k::Operand{ 0, assembly.instructions.back().argument2->type, 0, 0 },
					m68k::Operand{ -1, m68k::OperandType::REGISTER_sp_INDIRECT, 0, 0 },
					{}
				}
			);

			generate( *node.lhsValue, assembly );
			// If LHS was an address then dereference the address from a0 into d0
			if( assembly.instructions.back().argument2->type == m68k::OperandType::REGISTER_a0 ) {
				assembly.instructions.push_back(
					m68k::Instruction {
						m68k::Operator::MOVE,
						getDereferenceSize( assembly.instructions.back().metadata ),
						m68k::Operand { 0, m68k::OperandType::REGISTER_a0_INDIRECT, 0, 0 },
						m68k::Operand { 0, m68k::OperandType::REGISTER_d0, 0, 0 },
						{}
					}
				);
			}

			// LHS is now in d0
			// Promote type if needed
			if( getTypeComparison( lhsType ) < getTypeComparison( rhsType ) ) {
				// Promote to RHS type by clearing the upper bits of the register
				assembly.instructions.push_back(
					m68k::Instruction {
						m68k::Operator::AND,
						typeToWordSize( lhsType ),
						m68k::Operand{ 0, m68k::OperandType::IMMEDIATE, 0, getMaskByType( typeToWordSize( lhsType ) ) },
						m68k::Operand{ 0, m68k::OperandType::REGISTER_d0, 0, 0 },
						{}
					}
				);
			}

			// Prepare operator
			m68k::Operator op;
			switch( tokenOp ) {
				case TokenType::TOKEN_PLUS:
					op = m68k::Operator::ADD;
					break;
				case TokenType::TOKEN_MINUS:
					op = m68k::Operator::SUBTRACT;
					break;
				case TokenType::TOKEN_ASTERISK:
					op = isSigned( type ) ? m68k::Operator::MULTIPLY_SIGNED : m68k::Operator::MULTIPLY_UNSIGNED;
					break;
				case TokenType::TOKEN_FORWARD_SLASH:
					op = isSigned( type ) ? m68k::Operator::DIVIDE_SIGNED : m68k::Operator::DIVIDE_UNSIGNED;
					break;
				default:
					Error{ "Expected: ., +, -, *, or / operator", attemptToken( *node.op ) }.throwException();
			}

			// Apply operation from RHS, which currently sits on the stack
			if( rhsIsAddress ) {
				// RHS was a pointer
				// Pop from stack
				assembly.instructions.push_back(
					m68k::Instruction {
						m68k::Operator::MOVE,
						m68k::OperatorSize::LONG,
						m68k::Operand { 0, m68k::OperandType::REGISTER_sp, 1, 0 },
						m68k::Operand { 0, m68k::OperandType::REGISTER_a0, 0, 0 },
						{}
					}
				);

				// If the type needs to be promoted, move value into d1, promote in d1, and apply d1 as operation.
				// Otherwise, dereference a0 directly
				if( getTypeComparison( rhsType ) < getTypeComparison( lhsType ) ) {
					assembly.instructions.push_back(
						m68k::Instruction {
							m68k::Operator::MOVE,
							getDereferenceSize( rhsMetadata ),
							m68k::Operand{ 0, m68k::OperandType::REGISTER_a0_INDIRECT, 0, 0 },
							m68k::Operand{ 0, m68k::OperandType::REGISTER_d1, 0, 0 },
							{}
						}
					);

					assembly.instructions.push_back(
						m68k::Instruction {
							m68k::Operator::AND,
							getDereferenceSize( rhsMetadata ),
							m68k::Operand{ 0, m68k::OperandType::IMMEDIATE, 0, getMaskByType( getDereferenceSize( rhsMetadata ) ) },
							m68k::Operand{ 0, m68k::OperandType::REGISTER_d1, 0, 0 },
							{}
						}
					);

					assembly.instructions.push_back(
						m68k::Instruction {
							op,
							wordSize,
							m68k::Operand { 0, m68k::OperandType::REGISTER_d1, 0, 0 },
							m68k::Operand { 0, m68k::OperandType::REGISTER_d0, 0, 0 },
							{}
						}
					);
				} else {
					assembly.instructions.push_back(
						m68k::Instruction {
							op,
							wordSize,
							m68k::Operand { 0, m68k::OperandType::REGISTER_a0_INDIRECT, 0, 0 },
							m68k::Operand { 0, m68k::OperandType::REGISTER_d0, 0, 0 },
							{}
						}
					);
				}
			} else {
				// RHS was a literal
				// The value can be popped and used directly

				// If the value needs to be promoted, use d1
				if( getTypeComparison( rhsType ) < getTypeComparison( lhsType ) ) {
					assembly.instructions.push_back(
						m68k::Instruction {
							m68k::Operator::MOVE,
							typeToWordSize( rhsType ),
							m68k::Operand { 0, m68k::OperandType::REGISTER_sp_INDIRECT, 1, 0 },
							m68k::Operand { 0, m68k::OperandType::REGISTER_d1, 0, 0 },
							{}
						}
					);

					assembly.instructions.push_back(
						m68k::Instruction {
							m68k::Operator::AND,
							typeToWordSize( rhsType ),
							m68k::Operand { 0, m68k::OperandType::IMMEDIATE, 0, getMaskByType( typeToWordSize( rhsType ) ) },
							m68k::Operand { 0, m68k::OperandType::REGISTER_d1, 0, 0 },
							{}
						}
					);

					assembly.instructions.push_back(
						m68k::Instruction {
							op,
							wordSize,
							m68k::Operand { 0, m68k::OperandType::REGISTER_d1, 0, 0 },
							m68k::Operand { 0, m68k::OperandType::REGISTER_d0, 0, 0 },
							{}
						}
					);
				} else {
					assembly.instructions.push_back(
						m68k::Instruction {
							op,
							wordSize,
							m68k::Operand { 0, m68k::OperandType::REGISTER_sp_INDIRECT, 1, 0 },
							m68k::Operand { 0, m68k::OperandType::REGISTER_d0, 0, 0 },
							{}
						}
					);
				}
			}
		}
	}

	static void generate( const Expression& node, Assembly& assembly ) {

		if( auto binaryExpression = std::get_if< std::unique_ptr< BinaryExpression > >( &node.value ) ) {
			generate( **binaryExpression, assembly );
		}

	}

	VariantResult< Assembly > generate( const Program& program ) {
		return "Not implemented";
	}

}