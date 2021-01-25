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

	Result< Program, std::string > fileToTree( const std::string& parseFilename, CompilerSettings settings ) {
		auto fileResult = Utility::fileToString( parseFilename );

		if( auto file = std::get_if< Utility::File >( &fileResult ) ) {
			settings.symbols.addFile( parseFilename );

			auto tokenResult = getTokens( file->contents );
			if( auto tokens = std::get_if< std::vector< Token > >( &tokenResult ) ) {
				printSuccess( "Lexed file " + parseFilename );

				if( settings.printLex ) {
					for( const Token& token : *tokens ) {
						std::cout << token.toString() << std::endl;
					}
				}

				auto parserResult = getProgram( *tokens );
				if( auto program = std::get_if< Program >( &parserResult ) ) {
					printSuccess( "Parsed file " + parseFilename );

					if( settings.printAst ) {
						GoldScorpion::printAst( *program );
					}

					return Result< Program, std::string >::good( std::move( *program ) );
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

	Result< Program, std::string > fileToProgram( const std::string& parseFilename, CompilerSettings settings ) {
		// Do not do a thing if this file was opened before it was done
		if( settings.activeFiles.count( parseFilename ) ) {
			return Result< Program, std::string >::err( "Circular dependency detected: " + parseFilename );
		}

		// Mark file active while processing
		// If we try to reload this file before it was processed, it will throw a circular dependency error
		settings.activeFiles.insert( parseFilename );

		// Lex and parse file
		Result< Program, std::string > tree = fileToTree( parseFilename, settings );

		// Hand down error if it's an error
		if( !tree ) {
			return tree;
		}

		for( const auto& statement : ( *tree ).statements ) {
			// Expose file to SymbolResolver so that symbols can be properly exposed
			if( auto importDeclaration = std::get_if< std::unique_ptr< ImportDeclaration > >( &statement->value ) ) {
				// Don't reload the file if it was already active
				if( !settings.resolvedFiles.count( ( *importDeclaration )->path ) ) {
					auto result = fileToProgram( ( *importDeclaration )->path, settings );
					if( !result ) {
						return result;
					}
				}
			}
		}

		// Validate this file
		if( auto error = check( parseFilename, *tree, settings.symbols ) ) {
			return Result< Program, std::string >::err( "Failed to validate file " + parseFilename + ": " + *error );
		} else {
			printSuccess( "Validated file " + parseFilename );
			settings.activeFiles.erase( parseFilename );
			settings.resolvedFiles.insert( parseFilename );
			return Result< Program, std::string >::good( std::move( *tree ) );
		}
	}

    int compile( const std::string& parseFilename, bool printLex, bool printAst ) {
		SymbolResolver symbols;
		std::set< std::string > activeFiles;
		std::set< std::string > resolvedFiles;

		Result< Program, std::string > result = fileToProgram( parseFilename, CompilerSettings{ activeFiles, resolvedFiles, symbols, printLex, printAst } );

		if( !result ) {
			printError( result.getError() );
			return 1;
		}

		return 0;
    }

}