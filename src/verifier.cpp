#include "verifier.hpp"
#include "error.hpp"
#include <variant>

namespace GoldScorpion {

    static void expectTokenString( const Token& token, const std::string& error ) {
        if( !token.value || !std::holds_alternative< std::string >( *token.value ) ) {
           Error{ error, token }.throwException();
        }
    }

    static void expectTokenLong( const Token& token, const std::string& error ) {
        if( !token.value || !std::holds_alternative< long >( *token.value ) ) {
           Error{ error, token }.throwException();
        }
    }

    static void expectTokenValue( const Token& token, const std::string& error ) {
        if( !token.value ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenType( const Token& token, const std::string& error ) {
        if( token.type != TokenType::TOKEN_U8 ||
            token.type != TokenType::TOKEN_U16 ||
            token.type != TokenType::TOKEN_U32 ||
            token.type != TokenType::TOKEN_S8 ||
            token.type != TokenType::TOKEN_S16 ||
            token.type != TokenType::TOKEN_S32 ||
            token.type != TokenType::TOKEN_STRING ||
            token.type != TokenType::TOKEN_IDENTIFIER ||
            ( token.type == TokenType::TOKEN_IDENTIFIER && ( !token.value || !std::holds_alternative< std::string >( *token.value ) ) )
        ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenOfType( const Token& token, const TokenType& type, const std::string& error ) {
        if( token.type != type ) {
            Error{ error, token }.throwException();
        }
    }

    static void check( const VarDeclaration& node, MemoryTracker& memory ) {
        // def x as type = value

        // Verify x is an identifier containing a string
        expectTokenOfType( node.variable.name, TokenType::TOKEN_IDENTIFIER, "Expected: Identifier as token type for parameter declaration" );
        expectTokenString( node.variable.name, "Internal compiler error" );

        // Verify type is either primitive or declared
        expectTokenType( node.variable.type.type, "Expected: User-defined type or one of [u8, u16, u32, s8, s16, s32, string]" );

        // If type is user-defined type (IDENTIFIER) then we must verify the UDT was declared
        if( node.variable.type.type.type == TokenType::TOKEN_IDENTIFIER ) {
            std::string typeId = std::get< std::string >( *node.variable.type.type.value );
            if( !memory.findUdt( typeId ) ) {
                Error{ "Undeclared user-defined type: " + typeId, node.variable.type.type }.throwException();
            }
        }
    }

    /**
     * Run a verification step to make sure items are logically consistent
     * Additionally, build a MemoryTracker object that will contain registered UDTs encountered
     */
    Result< MemoryTracker > check( const Program& program ) {
        return "Not implemented";
    }

}