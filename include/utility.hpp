#pragma once
#include "result_type.hpp"

namespace GoldScorpion::Utility {

	struct File {
		std::string contents;
	};

	Result< File > fileToString( const std::string& filename );

}