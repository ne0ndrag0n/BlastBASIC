#pragma once
#include "result_type.hpp"
#include "memory_tracker.hpp"
#include "ast.hpp"
#include "error.hpp"
#include <optional>

namespace GoldScorpion {

    Result< MemoryTracker > check( const Program& program );

}