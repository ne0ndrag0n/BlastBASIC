#pragma once
#include "arch/m68k/instruction.hpp"
#include "memory_tracker.hpp"
#include <vector>

namespace GoldScorpion {

    struct Assembly {
		std::vector< m68k::Instruction > instructions;
		MemoryTracker memory;
	};

}