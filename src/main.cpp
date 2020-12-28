#include "ast.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include "utility.hpp"
#include "variant_visitor.hpp"
#include "visitor_print.hpp"
#include "verifier.hpp"
#include <rang.hpp>
#include <CLI11.hpp>

void info() {
	std::cout << rang::fg::yellow << "GoldScorpion " << rang::style::reset << "Embedded Software Development Kit" << std::endl;
	std::cout << "Build 0000-00-00:00 (c) Ashley N. 2020" << std::endl;
	std::cout << rang::fgB::cyan << "Target Architecture: " << rang::style::reset << "m68k-md" << std::endl;
	std::cout << std::endl;
	std::cout << "https://www.github.com/ne0ndrag0n/GoldScorpion" << std::endl;
	std::cout << std::endl;
}

void printError( const std::string& message ) {
	std::cout << rang::fgB::red << "error: " << rang::style::reset << message << std::endl;
}

void printSuccess( const std::string& message ) {
	std::cout << rang::fgB::green << "success: " << rang::style::reset << message << std::endl;
}

int main( int argc, char** argv ) {
	std::string parseFilename;
	bool printLex = false;
	bool printAst = false;

	CLI::App application{ "GoldScorpion Embedded SDK v0.0.1 [m68k-md]" };

	application.add_flag_callback( "-i,--info", info, "Print info about this build" );
	application.add_flag( "--debug-lex", printLex, "Print lexer output for file" );
	application.add_flag( "--debug-parse", printAst, "Print parse tree output for file" );
	application.add_option( "-f,--file", parseFilename, "Specify input file" );
	application.add_option( "-o,--output", "Specify output ROM" );
	application.add_option( "-a,--assembler-path", "Specify path to target assembler" );

	CLI11_PARSE( application, argc, argv );

	if( parseFilename.empty() ) {
		printError( "no input files" );
		return 1;
	} else {
		auto fileResult = GoldScorpion::Utility::fileToString( parseFilename );
		if( auto file = std::get_if< GoldScorpion::Utility::File >( &fileResult ) ) {
			auto tokenResult = GoldScorpion::getTokens( file->contents );

			if( auto tokens = std::get_if< std::vector< GoldScorpion::Token > >( &tokenResult ) ) {
				printSuccess( "Lexed file " + parseFilename );

				if( printLex ) {
					for( const GoldScorpion::Token& token : *tokens ) {
						std::cout << token.toString() << std::endl;
					}
				}

				// Try this and see if anything crashes
				auto parserResult = GoldScorpion::getProgram( *tokens );
				if( auto program = std::get_if< GoldScorpion::Program >( &parserResult ) ) {
					printSuccess( "Parsed file " + parseFilename );

					if( printAst ) {
						GoldScorpion::printAst( *program );
					}

					if( auto error = check( *program ) ) {
						printError( "Failed to validate file " + parseFilename + ": " + *error );
						return 5;
					} else {
						printSuccess( "Validated file " + parseFilename );
					}

					return 0;
				} else {
					std::string error = std::get< std::string >( std::move( parserResult ) );
					printError( "Could not parse file " + parseFilename + ": " + error );
					return 4;
				}

			} else {
				printError( "Could not lex file " + parseFilename + ": " + std::get< std::string >( tokenResult ) );
				return 3;
			}

		} else {
			printError( "Could not open file " + parseFilename + ": " + std::get< std::string >( fileResult ) );
			return 2;
		}
	}


	return 0;
}