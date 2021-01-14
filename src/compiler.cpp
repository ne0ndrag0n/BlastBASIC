#include "ast.hpp"
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

    int compile( const std::string& parseFilename, bool printLex, bool printAst ) {
		auto fileResult = GoldScorpion::Utility::fileToString( parseFilename );

		if( auto file = std::get_if< GoldScorpion::Utility::File >( &fileResult ) ) {
			auto tokenResult = GoldScorpion::getTokens( file->contents );

			if( auto tokens = std::get_if< std::vector< GoldScorpion::Token > >( &tokenResult ) ) {
				GoldScorpion::printSuccess( "Lexed file " + parseFilename );

				if( printLex ) {
					for( const GoldScorpion::Token& token : *tokens ) {
						std::cout << token.toString() << std::endl;
					}
				}

				// Try this and see if anything crashes
				auto parserResult = GoldScorpion::getProgram( *tokens );
				if( auto program = std::get_if< GoldScorpion::Program >( &parserResult ) ) {
					GoldScorpion::printSuccess( "Parsed file " + parseFilename );

					if( printAst ) {
						GoldScorpion::printAst( *program );
					}

					if( auto error = check( *program ) ) {
						GoldScorpion::printError( "Failed to validate file " + parseFilename + ": " + *error );
						return 5;
					} else {
						GoldScorpion::printSuccess( "Validated file " + parseFilename );
					}

					return 0;
				} else {
					std::string error = std::get< std::string >( std::move( parserResult ) );
					GoldScorpion::printError( "Could not parse file " + parseFilename + ": " + error );
					return 4;
				}

			} else {
				GoldScorpion::printError( "Could not lex file " + parseFilename + ": " + std::get< std::string >( tokenResult ) );
				return 3;
			}

		} else {
			GoldScorpion::printError( "Could not open file " + parseFilename + ": " + std::get< std::string >( fileResult ) );
			return 2;
		}

        return 0;
    }

}