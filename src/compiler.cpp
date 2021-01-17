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

	Result< Program, std::string > fileToProgram( const std::string& parseFilename, bool printLex, bool printAst ) {
		auto fileResult = Utility::fileToString( parseFilename );

		if( auto file = std::get_if< Utility::File >( &fileResult ) ) {
			auto tokenResult = getTokens( file->contents );

			if( auto tokens = std::get_if< std::vector< Token > >( &tokenResult ) ) {
				printSuccess( "Lexed file " + parseFilename );

				if( printLex ) {
					for( const Token& token : *tokens ) {
						std::cout << token.toString() << std::endl;
					}
				}

				// Try this and see if anything crashes
				auto parserResult = getProgram( *tokens );
				if( auto program = std::get_if< Program >( &parserResult ) ) {
					printSuccess( "Parsed file " + parseFilename );

					if( printAst ) {
						GoldScorpion::printAst( *program );
					}

					if( auto error = check( *program ) ) {
						return Result< Program, std::string >::err( "Failed to validate file " + parseFilename + ": " + *error );
					} else {
						// Iterate through first-level of freshly-parsed, freshly-checked tree and replace ImportDeclarations with subtrees
						for( unsigned int i = 0; i < program->statements.size(); i++ ) {
							const std::unique_ptr< Declaration >& declaration = program->statements[ i ];

							// Only care about ImportDeclaration
							if( auto importDeclaration = std::get_if< std::unique_ptr< ImportDeclaration > >( &declaration->value ) ) {
								// Attempt to convert path to new AST
								Result< Program, std::string > subresult = fileToProgram( (*importDeclaration)->path, printLex, printAst );
								if( subresult ) {
									// Erase the original item
									program->statements.erase( program->statements.begin() + i );

									// Insert new items at each i, incrementing i each time
									for( std::unique_ptr< Declaration >& declaration : (*subresult).statements ) {
										program->statements.insert( program->statements.begin() + i, std::move( declaration ) );
										// These statements were already validated and are being added to the current list of statements
										// Skip past them
										i++;
									}
								} else {
									return Result< Program, std::string >::err( "Could not import file " + (*importDeclaration)->path + ": " + subresult.getError()  );
								}
							}
						}

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
		Result< Program, std::string > result = fileToProgram( parseFilename, printLex, printAst );

		if( !result ) {
			printError( result.getError() );
			return 1;
		}

		return 0;
    }

}