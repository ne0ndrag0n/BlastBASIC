#pragma once
#include "memory_tracker.hpp"
#include "ast.hpp"
#include "error.hpp"
#include <optional>

namespace GoldScorpion {

    std::optional< std::string > check( const std::string& fileId, const Program& program );

}