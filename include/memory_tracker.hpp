#pragma once
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <stack>

namespace GoldScorpion {

	struct FunctionTypeParameter {
		std::string id;
		std::string typeId;
	};

	struct FunctionType {
		std::string id;
		std::vector< FunctionTypeParameter > arguments;
		std::string returnTypeId;
	};

	struct ValueType {
		std::string id;
	};
	using MemoryDataType = std::variant< FunctionType, ValueType >;

	struct UdtField {
		std::string id;
		MemoryDataType type;
	};

	struct UserDefinedType {
		std::string id;
		std::vector< UdtField > fields;
	};

	struct MemoryElement {
		std::optional< std::string > id;
		MemoryDataType type;
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
	struct Scope {
		long stackItems;
		long udtItems;
	};

	class MemoryTracker {
		std::vector< MemoryElement > dataSegment;
		std::vector< MemoryElement > stack;
		std::vector< UserDefinedType > udts;
		std::stack< Scope > scopes;

	public:
		void insert( MemoryElement element );

		void push( const MemoryElement& element );
		std::optional< MemoryElement > pop();

		void clearMemory();

		void openScope();
		std::vector< StackMemoryElement > closeScope();

		std::optional< MemoryQuery > find( const std::string& id, bool currentScope = false ) const;

		void addUdt( const UserDefinedType& udt );
		std::optional< UserDefinedType > findUdt( const std::string& id, bool currentScope = false ) const;
		std::optional< UdtField > findUdtField( const std::string& id, const std::string& fieldId, bool currentScope = false ) const;

		static MemoryElement unwrapValue( const MemoryQuery& query );
		static long unwrapOffset( const MemoryQuery& query );

		static std::string unwrapTypeId( const MemoryDataType& type );
	};

}