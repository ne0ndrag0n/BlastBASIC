#include "ast.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include "utility.hpp"
#include "variant_visitor.hpp"
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

	CLI::App application{ "GoldScorpion v0.0.1 - Ferociously Easy Embedded Systems Programming" };

	application.add_flag_callback( "-i,--info", info, "Print info about this build" );
	application.add_option( "-f,--file", parseFilename, "Specify input file" );

	CLI11_PARSE( application, argc, argv );

	if( parseFilename.empty() ) {
		std::cout << rang::fgB::red << "error: " << rang::style::reset << "no input files." << std::endl;
		return 1;
	}

	/*
	auto result = GoldScorpion::getProgram( parseFilename );
	std::visit( overloaded {
		[]( const GoldScorpion::Program& program ) {
			std::cout << "Successfully parsed" << std::endl;
		},
		[]( const std::string& error ) {
			std::cout << rang::fgB::red << "error: " << rang::style::reset << error << std::endl;
		}
	}, result );
	*/

	// Example code to test lexer
	else {
		auto fileResult = GoldScorpion::Utility::fileToString( parseFilename );

		if( auto file = std::get_if< GoldScorpion::Utility::File >( &fileResult ) ) {
			auto tokenResult = GoldScorpion::getTokens( file->contents );

			if( auto tokens = std::get_if< std::vector< GoldScorpion::Token > >( &tokenResult ) ) {
				std::cout << rang::fgB::green << "success: " << rang::style::reset
					<< "Parsed file " << parseFilename << std::endl;

				for( const GoldScorpion::Token& token : *tokens ) {
					std::cout << token.toString() << std::endl;
				}

				return 0;
			} else {
				std::cout << rang::fgB::red << "error: " << rang::style::reset
					<< "Could not parse file " << parseFilename << ": " <<
					std::get< std::string >( tokenResult ) << std::endl;

				return 3;
			}

		} else {
			std::cout << rang::fgB::red << "error: " << rang::style::reset
				<< "Could not open file " << parseFilename << ": " <<
				std::get< std::string >( fileResult ) << std::endl;

			return 2;
		}
	}


	return 0;
}