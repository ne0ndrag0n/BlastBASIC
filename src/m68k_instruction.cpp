#include "arch/m68k/instruction.hpp"
#include <cstdio>
#include <regex>

namespace GoldScorpion::m68k {
	

	static std::string operatorSizeToDirective( OperatorSize size ) {
		switch( size ) {
			case OperatorSize::BYTE:
				return "b";
			case OperatorSize::WORD:
			default:
				return "w";
			case OperatorSize::LONG:
				return "l";
		}
	}

	static std::string operandTypeToDirective( OperandType type ) {
		switch( type ) {
			default:
			case OperandType::REGISTER_d0:
				return "d0";
			case OperandType::REGISTER_d1:
				return "d1";
			case OperandType::REGISTER_a0:
				return "a0";
			case OperandType::REGISTER_a1:
				return "a1";
			case OperandType::REGISTER_sp:
				return "sp";
			case OperandType::REGISTER_fp:
				return "fp";
			case OperandType::IMMEDIATE:
				return "#value";
			case OperandType::INDIRECT:
				return "value";
		}
	}

	std::string Operand::toString() const {
		std::string result;

		if( preIncrement ) {
			if( preIncrement == 1 ) {
				result += "+(";
			} else if ( preIncrement == -1 ) {
				result += "-(";
			} else {
				result += preIncrement + "(";
			}
		}

		result += operandTypeToDirective( type );

		if( postIncrement ) {
			if( postIncrement == 1 ) {
				result += ")+";
			} else if( postIncrement == -1 ) {
				result += ")-";
			} else {
				result += ")" + postIncrement;
			}
		}

		result = std::regex_replace( result, std::regex( "value" ), std::to_string( value ) );

		return result;
	}

	std::string Instruction::toString() const {
		// Get instruction name
		std::string instruction = "";
		switch( op ) {
			default:
			case Operator::MOVE:
				instruction = "move." + operatorSizeToDirective( size );
				break;
			case Operator::ADD:
				instruction = "add." + operatorSizeToDirective( size );
				break;
			case Operator::SUBTRACT:
				instruction = "sub." + operatorSizeToDirective( size );
				break;
			case Operator::MULTIPLY_SIGNED:
				instruction = "muls." + operatorSizeToDirective( size );
				break;
			case Operator::MULTIPLY_UNSIGNED:
				instruction = "mulu." + operatorSizeToDirective( size );
				break;
			case Operator::DIVIDE_SIGNED:
				instruction = "divs." + operatorSizeToDirective( size );
				break;
			case Operator::DIVIDE_UNSIGNED:
				instruction = "divu." + operatorSizeToDirective( size );
				break;
		}

		instruction += "\t";

		instruction += argument1.toString();

		if( argument2 ) {
			instruction += ", ";

			instruction += argument2->toString();
		}

		return instruction;
	}
}