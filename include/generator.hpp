#pragma once
#include "result_type.hpp"
#include "assembly.hpp"
#include <ast.hpp>
#include <string>

namespace GoldScorpion {

	Result< Assembly > generate( const Program& program );

}