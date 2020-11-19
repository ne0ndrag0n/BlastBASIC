#pragma once
#include "ast.hpp"
#include "token.hpp"
#include "result_type.hpp"
#include <vector>

namespace GoldScorpion {

	Result< Program > getProgram( std::vector< Token > filename );

}