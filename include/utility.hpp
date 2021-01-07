#pragma once
#include "result_type.hpp"
#include <vector>
#include <string>

namespace GoldScorpion::Utility {

	struct File {
		std::string contents;
	};

	static constexpr unsigned int hash( const char* str, int h = 0 ) {
		if( !str ) {
			str = "";
		}

		return !str[ h ] ? 5381 : ( hash( str, h+1 ) * 33 ) ^ str[ h ];
	};

	std::string stringLtrim( std::string& s );
	std::string stringRtrim( std::string& s );
	std::string stringTrim( std::string s );

	std::vector< std::string > split( const std::string &text, char sep );

	VariantResult< File > fileToString( const std::string& filename );

	std::string longToHex( long value );

}