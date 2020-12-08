#pragma once
#include "result_type.hpp"
#include <ast.hpp>
#include <string>

namespace GoldScorpion {

	struct Assembly {
		std::string body;
	};

	Result< Assembly > generate( const Program& program );

}