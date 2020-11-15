#pragma once
#include <variant>
#include <string>

namespace GoldScorpion {

	template< typename T >
	using Result = std::variant< T, std::string >;

}