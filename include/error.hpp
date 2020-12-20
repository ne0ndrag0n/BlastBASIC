#pragma once
#include "token.hpp"
#include <string>
#include <optional>

namespace GoldScorpion {

	struct Error {
		std::string text;
		std::optional< Token > near;

		std::string toString() const;
		void throwException() const;
	};

}