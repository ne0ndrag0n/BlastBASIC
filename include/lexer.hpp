#pragma once
#include "token.hpp"
#include "result_type.hpp"
#include <vector>

namespace GoldScorpion {

	VariantResult< std::vector< Token > > getTokens( std::string body );

}