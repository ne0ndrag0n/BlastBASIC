#pragma once
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace GoldScorpion {

	struct UdtField {
		std::string id;
		int size;
	};

	struct UserDefinedType {
		std::string id;
		std::vector< UdtField > fields;
	};

	struct MemoryElement {
		std::optional< std::string > id;
		std::string typeId;
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
		std::vector< UserDefinedType > udts;

	public:
		void insert( MemoryElement element );

		void push( const MemoryElement& element );
		MemoryElement pop();

		std::optional< MemoryQuery > find( const std::string& id ) const;

		void addUdt( const UserDefinedType& udt );
		std::optional< UserDefinedType > findUdt( const std::string& id ) const;
		std::optional< UdtField > findUdtField( const std::string& id, const std::string& fieldId ) const;

		static MemoryElement unwrapValue( const MemoryQuery& query );
		static long unwrapOffset( const MemoryQuery& query );
	};

}