#include "utility.hpp"
#include <fstream>
#include <sstream>
#include <exception>
#include <string>
#include <cstdio>

namespace GoldScorpion::Utility {

	std::string stringLtrim( std::string& s ) {
		if( s.size() == 0 ) {
			return "";
		}

		return s.substr( s.find_first_not_of( " \t\f\v\n\r" ) );
	}

	std::string stringRtrim( std::string& s ) {
		if( s.size() == 0 ) {
			return "";
		}

		return s.erase( s.find_last_not_of( " \t\f\v\n\r" ) + 1 );
	}

	std::string stringTrim( std::string s ) {
		std::string rTrimmed = stringRtrim( s );

		return stringLtrim( rTrimmed );
	}

	std::vector< std::string > split( const std::string &text, char sep ) {
		std::vector<std::string> tokens;
		std::size_t start = 0, end = 0;
		while ((end = text.find(sep, start)) != std::string::npos) {
			tokens.push_back(text.substr(start, end - start));
			start = end + 1;
		}
		tokens.push_back(text.substr(start));
		return tokens;
	}

	VariantResult< File > fileToString( const std::string& filename ) {
		File result;

		try {
			std::ifstream stream( stringTrim( filename ) );
			if( stream.is_open() && stream.good() ) {
				std::stringstream stringBuilder;
				stringBuilder << stream.rdbuf();

				result.contents = stringBuilder.str();
			} else {
				return "Unable to open file: ifstream was not open or good";
			}
		} catch( std::exception e ) {
			return std::string( "Unable to open file: " ) + e.what();
		}

		return result;
	}

	std::string longToHex( long value ) {
		char buffer[ 25 ] = { 0 };

		std::sprintf( buffer, "%08lX", value );

		return buffer;
	}

}