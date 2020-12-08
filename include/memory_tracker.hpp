#pragma once
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace GoldScorpion {

	struct MemoryElement {
		std::optional< std::string > id;
		int size;
		long value;
	};

	// *cries in lack of algebraic data types*
	struct GlobalMemoryElement {
		MemoryElement value;
		long offset;
	};

	struct StackMemoryElement {
		MemoryElement value;
		long offset;
	};

	using MemoryQuery = std::variant< GlobalMemoryElement, StackMemoryElement >;

	class MemoryTracker {
		std::vector< MemoryElement > dataSegment;
		std::vector< MemoryElement > stack;

	public:
		void insert( MemoryElement element );

		void push( const MemoryElement& element );
		MemoryElement pop();

		std::optional< MemoryQuery > find( const std::string& id ) const;
	};

}