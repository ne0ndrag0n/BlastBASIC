#pragma once
#include <string>
#include <optional>
#include <set>

namespace GoldScorpion::m68k {
	
	enum class Operator {
		MOVE,
		ADD,
		SUBTRACT,
		MULTIPLY_SIGNED,
		MULTIPLY_UNSIGNED,
		DIVIDE_SIGNED,
		DIVIDE_UNSIGNED,
		AND,
		OR,
		NOT
	};

	enum class OperatorSize {
		DEFAULT,
		BYTE,
		WORD,
		LONG
	};

	enum class OperandType {
		REGISTER_d0,
		REGISTER_d1,
		REGISTER_a0,
		REGISTER_a0_INDIRECT,
		REGISTER_a1,
		REGISTER_a1_INDIRECT,
		REGISTER_sp,
		REGISTER_sp_INDIRECT,
		REGISTER_fp,
		REGISTER_fp_INDIRECT,
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

	enum class InstructionMetadata {
		POINTER,
		POINTER_BYTE,
		POINTER_WORD,
		POINTER_LONG,
		POINTER_STRING
	};

	struct Instruction {
		Operator op;
		OperatorSize size = OperatorSize::WORD;
		Operand argument1;
		std::optional< Operand > argument2;
		std::set< InstructionMetadata > metadata;

		std::string toString() const;
	};

}