#pragma once
#include "result_type.hpp"
#include "memory_tracker.hpp"
#include "arch/m68k/instruction.hpp"
#include <ast.hpp>
#include <string>

namespace GoldScorpion {

	struct Assembly {
		std::vector< m68k::Instruction > instructions;
		MemoryTracker memory;
	};

	Result< Assembly > generate( const Program& program );

}