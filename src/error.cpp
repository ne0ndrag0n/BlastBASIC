#include "error.hpp"
#include <exception>

namespace GoldScorpion {

	std::string Error::toString() const {
		return near ?
			( std::string( "At " ) + std::to_string( near->line ) + ", " + std::to_string( near->column ) + ": " + text )
		  	: text;
	}

	void Error::throwException() const {
		throw std::runtime_error( toString() );
	}

}