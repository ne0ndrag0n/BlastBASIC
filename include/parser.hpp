#pragma once
#include "ast.hpp"
#include "token.hpp"
#include "result_type.hpp"
#include <queue>

namespace GoldScorpion {

	Result< Program > getProgram( std::queue< Token > filename );

}