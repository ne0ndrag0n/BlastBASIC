#include "memory_tracker.hpp"

namespace GoldScorpion {

	void MemoryTracker::insert( MemoryElement element ) {
		dataSegment.push_back( element );
	}

	void MemoryTracker::push( const MemoryElement& element ) {
		stack.push_back( element );
	}

	MemoryElement MemoryTracker::pop() {
		MemoryElement result = stack.back();

		stack.pop_back();

		return result;
	}

	std::optional< MemoryQuery > MemoryTracker::find( const std::string& id ) const {
		// When asked to find an elment, find from innermost scope to outermost scope

		// Step 1: Stack, from top down
		long offset = 0;
		for( auto entry = stack.crbegin(); entry != stack.crend(); ++entry ) {
			if( entry->id && *entry->id == id ) {
				return StackMemoryElement {
					*entry,
					offset
				};
			} else {
				offset += entry->size;
			}
		}

		// The variable wasn't found on the stack
		// Step 2: Search the application data segment
		offset = 0;
		for( const auto& entry : dataSegment ) {
			if( entry.id && *entry.id == id ) {
				return GlobalMemoryElement {
					entry,
					offset
				};
			} else {
				offset += entry.size;
			}
		}

		// No result
		return {};
	}

}