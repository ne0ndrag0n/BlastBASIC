#pragma once
#include "ast.hpp"
#include "token.hpp"
#include "result_type.hpp"
#include <vector>

namespace GoldScorpion {

	VariantResult< Program > getProgram( std::vector< Token > tokens );

}