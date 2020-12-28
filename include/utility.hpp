#pragma once
#include "result_type.hpp"

namespace GoldScorpion::Utility {

	struct File {
		std::string contents;
	};

	std::string stringLtrim( std::string& s );
	std::string stringRtrim( std::string& s );
	std::string stringTrim( std::string s );

	VariantResult< File > fileToString( const std::string& filename );

	std::string longToHex( long value );

}