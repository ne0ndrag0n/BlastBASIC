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

		// Push a pointer to this value onto the most recently opened scope
		if( !scopes.empty() ) {
			scopes.top().stackItems++;
		}
	}

	std::optional< MemoryElement > MemoryTracker::pop() {
		if( !stack.empty() ) {
			MemoryElement result = stack.back();
			stack.pop_back();

			if( !scopes.empty() ) {
				scopes.top().stackItems--;
			}

			return result;
		} else {
			return {};
		}
	}

	void MemoryTracker::clearMemory() {
		dataSegment.clear();
		stack.clear();
		udts.clear();
		while( !scopes.empty() ) {
			scopes.pop();
		}
	}

	void MemoryTracker::openScope() {
		scopes.push( Scope{ 0, 0 } );
	}

	std::vector< StackMemoryElement > MemoryTracker::closeScope() {
		std::vector< StackMemoryElement > elements;

		if( !scopes.empty() ) {
			long offset = 0;
			for( long i = 0; i != scopes.top().stackItems; i++ ) {
				elements.push_back( StackMemoryElement{ stack.back(), offset } );
				offset += stack.back().size;
				stack.pop_back();
			}

			for( long i = 0; i != scopes.top().udtItems; i++ ) {
				udts.pop_back();
			}

			scopes.pop();
		}

		return elements;
	}

	std::optional< MemoryQuery > MemoryTracker::find( const std::string& id, bool currentScope ) const {
		// When asked to find an elment, find from innermost scope to outermost scope

		// Step 1: Stack, from top down
		long offset = 0;
		long scopeCount = 0;
		for( auto entry = stack.crbegin(); entry != stack.crend(); ++entry ) {
			if( currentScope ) {
				if( !scopes.empty() && scopeCount < scopes.top().stackItems ) {
					scopeCount++;
				} else {
					break;
				}
			}

			if( entry->id && *entry->id == id ) {
				return StackMemoryElement {
					*entry,
					offset
				};
			} else {
				offset += entry->size;
			}
		}

		// If we're only searching the current scope then we need to stop here
		if( currentScope ) {
			return {};
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
		if( !scopes.empty() ) {
			scopes.top().udtItems++;
		}
	}

	std::optional< UserDefinedType > MemoryTracker::findUdt( const std::string& id, bool currentScope ) const {
		long scopeCount = 0;
		for( const auto& udt : udts ) {
			if( currentScope ) {
				if( !scopes.empty() && scopeCount < scopes.top().udtItems ) {
					scopeCount++;
				} else {
					break;
				}
			}

			if( udt.id == id ) {
				return udt;
			}
		}

		return {};
	}

	void MemoryTracker::addUdtField( const std::string& id, const UdtField& field, bool currentScope ) {
		long scopeCount = 0;
		for( auto& udt : udts ) {
			if( currentScope ) {
				if( !scopes.empty() && scopeCount < scopes.top().udtItems ) {
					scopeCount++;
				} else {
					break;
				}
			}

			if( udt.id == id ) {
				udt.fields.push_back( field );
			}
		}
	}

	std::optional< UdtField > MemoryTracker::findUdtField( const std::string& id, const std::string& fieldId, bool currentScope ) const {
		auto udt = findUdt( id, currentScope );

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