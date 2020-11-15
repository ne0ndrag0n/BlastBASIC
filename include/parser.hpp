#pragma once
#include "ast.hpp"
#include "result_type.hpp"

namespace GoldScorpion {

	Result< Program > getProgram( const std::string& filename );

}