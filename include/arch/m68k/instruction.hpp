#pragma once
#include <string>

namespace GoldScorpion::m68k {
	
	enum class Operator {
		MOVE,
		ADD,
		SUBTRACT,
		MULTIPLY_SIGNED,
		MULTIPLY_UNSIGNED,
		DIVIDE_SIGNED,
		DIVIDE_UNSIGNED
	};

	enum class OperatorSize {
		BYTE,
		WORD,
		LONG
	};

	enum class OperandType {
		REGISTER_d0,
		REGISTER_d1,
		REGISTER_a0,
		REGISTER_a1,
		REGISTER_sp,
		REGISTER_fp,
		IMMEDIATE,
		INDIRECT
	};

	struct Operand {
		OperandType type;
		int preIncrement;
		int postIncrement;
		long value;

		std::string toString() const;
	};

	struct Instruction {
		Operator op;
		OperatorSize size = OperatorSize::WORD;
		Operand argument1;
		Operand argument2;

		std::string toString() const;
	};

}