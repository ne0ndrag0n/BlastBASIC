#include "utility.hpp"
#include <fstream>
#include <sstream>
#include <exception>

namespace GoldScorpion::Utility {

	Result< File > fileToString( const std::string& filename ) {
		File result;

		try {
			std::ifstream stream( filename );
			if( stream.is_open() && stream.good() ) {
				std::stringstream stringBuilder;
				stringBuilder << stream.rdbuf();

				result.contents = stringBuilder.str();
			} else {
				return "Unable to open file";
			}
		} catch( std::exception e ) {
			return "Unable to open file";
		}

		return result;
	}

}