#pragma once
#include <string>
#include <optional>

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
		int preIncrement;
		OperandType type;
		int postIncrement;
		long value;

		std::string toString() const;
	};

	struct Instruction {
		Operator op;
		OperatorSize size = OperatorSize::WORD;
		Operand argument1;
		std::optional< Operand > argument2;

		std::string toString() const;
	};

}