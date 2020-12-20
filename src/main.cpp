#include "ast.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include "utility.hpp"
#include "variant_visitor.hpp"
#include "visitor_print.hpp"
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
		std::cout << rang::fgB::red << "error: " << rang::style::reset << "no input files." << std::endl;
		return 1;
	} else {
		auto fileResult = GoldScorpion::Utility::fileToString( parseFilename );
		if( auto file = std::get_if< GoldScorpion::Utility::File >( &fileResult ) ) {
			auto tokenResult = GoldScorpion::getTokens( file->contents );

			if( auto tokens = std::get_if< std::vector< GoldScorpion::Token > >( &tokenResult ) ) {
				std::cout << rang::fgB::green << "success: " << rang::style::reset <<
					"Lexed file " << parseFilename << std::endl;

				if( printLex ) {
					for( const GoldScorpion::Token& token : *tokens ) {
						std::cout << token.toString() << std::endl;
					}
				}

				// Try this and see if anything crashes
				auto parserResult = GoldScorpion::getProgram( *tokens );
				if( auto program = std::get_if< GoldScorpion::Program >( &parserResult ) ) {
					std::cout << rang::fgB::green << "success: " << rang::style::reset <<
						"Parsed file " << parseFilename << std::endl;

					if( printAst ) {
						GoldScorpion::printAst( *program );
					}

					return 0;
				} else {
					std::string error = std::get< std::string >( std::move( parserResult ) );
					std::cout << rang::fgB::red << "error: " << rang::style::reset <<
						"Could not parse file " << parseFilename << ": " <<
						error << std::endl;

					return 4;
				}

			} else {
				std::cout << rang::fgB::red << "error: " << rang::style::reset <<
					"Could not lex file " << parseFilename << ": " <<
					std::get< std::string >( tokenResult ) << std::endl;
				return 3;
			}

		} else {
			std::cout << rang::fgB::red << "error: " << rang::style::reset <<
				"Could not open file " << parseFilename << ": " <<
				std::get< std::string >( fileResult ) << std::endl;

			return 2;
		}
	}


	return 0;
}