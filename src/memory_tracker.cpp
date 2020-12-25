#include "memory_tracker.hpp"
#include "variant_visitor.hpp"

namespace GoldScorpion {

	MemoryElement MemoryTracker::unwrapValue( const MemoryQuery& query ) {
		return std::visit( overloaded {
			[]( const GlobalMemoryElement& element ) { return element.value; },
			[]( const StackMemoryElement& element ) { return element.value; }
		}, query );
	}

	long MemoryTracker::unwrapOffset( const MemoryQuery& query ) {
		return std::visit( overloaded {
			[]( const GlobalMemoryElement& element ) { return element.offset; },
			[]( const StackMemoryElement& element ) { return element.offset; }
		}, query );
	}

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

	void MemoryTracker::clearMemory() {
		dataSegment.clear();
		stack.clear();
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

	void MemoryTracker::addUdt( const UserDefinedType& udt ) {
		udts.push_back( udt );
	}

	std::optional< UserDefinedType > MemoryTracker::findUdt( const std::string& id ) const {
		for( const auto& udt : udts ) {
			if( udt.id == id ) {
				return udt;
			}
		}

		return {};
	}

	std::optional< UdtField > MemoryTracker::findUdtField( const std::string& id, const std::string& fieldId ) const {
		auto udt = findUdt( id );

		if( udt ) {
			for( const auto& udtField : udt->fields ) {
				if( udtField.id == fieldId ) {
					return udtField;
				}
			}
		}

		return {};
	}

}