#include "log.hpp"
#include "compiler.hpp"
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
		GoldScorpion::printError( "no input files" );
		return 1;
	} else {
		return GoldScorpion::compile( parseFilename, printLex, printAst );
	}
}