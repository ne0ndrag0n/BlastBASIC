#pragma once
#include <variant>
#include <string>
#include <optional>
#include <exception>

namespace GoldScorpion {

	template< typename T >
	using VariantResult = std::variant< T, std::string >;

	template< typename Object, typename Error >
	class Result {
		std::optional< Object > result;
		std::optional< Error > error;

	public:
		Result() {}

		static Result good( Object object ) {
			Result enhancedResult;
			enhancedResult.result = object;
			return enhancedResult;
		};

		static Result err( Error error ) {
			Result enhancedResult;
			enhancedResult.error = error;
			return enhancedResult;
		};

		explicit operator bool() const {
			return result.operator bool();
		}

		Object& operator*() {
			if( !result ) {
				throw std::runtime_error( "Attempted to dereference Result with error inside" );
			}

			return *result;
		}

		Object& operator->() {
			if( !result ) {
				throw std::runtime_error( "Attempted to dereference Result with error inside" );
			}

			return *result;
		}

		Error& getError() {
			if( !error ) {
				throw std::runtime_error( "Attempted to get error from Result containing no error" );
			}

			return *error;
		}
	};

}