#include "compiler.hpp"
#include "variant_visitor.hpp"
#include "utility.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "verifier.hpp"
#include "log.hpp"
#include "visitor_print.hpp"
#include <vector>
#include <utility>

namespace GoldScorpion {

	Result< Program, std::string > fileToProgram( const std::string& parseFilename, CompilerSettings settings ) {
		auto fileResult = Utility::fileToString( parseFilename );

		if( auto file = std::get_if< Utility::File >( &fileResult ) ) {
			auto tokenResult = getTokens( file->contents );

			if( auto tokens = std::get_if< std::vector< Token > >( &tokenResult ) ) {
				printSuccess( "Lexed file " + parseFilename );

				if( settings.printLex ) {
					for( const Token& token : *tokens ) {
						std::cout << token.toString() << std::endl;
					}
				}

				// Try this and see if anything crashes
				auto parserResult = getProgram( *tokens );
				if( auto program = std::get_if< Program >( &parserResult ) ) {
					printSuccess( "Parsed file " + parseFilename );

					if( settings.printAst ) {
						GoldScorpion::printAst( *program );
					}

					if( auto error = check( parseFilename, *program ) ) {
						return Result< Program, std::string >::err( "Failed to validate file " + parseFilename + ": " + *error );
					} else {
						printSuccess( "Validated file " + parseFilename );
						return Result< Program, std::string >::good( std::move( *program ) );
					}
				} else {
					return Result< Program, std::string >::err( "Could not parse file " + parseFilename + ": " + std::get< std::string >( std::move( parserResult ) ) );
				}

			} else {
				return Result< Program, std::string >::err( "Could not lex file " + parseFilename + ": " + std::get< std::string >( tokenResult ) );
			}

		} else {
			return Result< Program, std::string >::err( "Could not open file " + parseFilename + ": " + std::get< std::string >( fileResult ) );
		}
	}

    int compile( const std::string& parseFilename, bool printLex, bool printAst ) {
		SymbolResolver symbols;
		std::set< std::string > activeFiles;

		Result< Program, std::string > result = fileToProgram( parseFilename, CompilerSettings{ activeFiles, symbols, printLex, printAst } );

		if( !result ) {
			printError( result.getError() );
			return 1;
		}

		return 0;
    }

}