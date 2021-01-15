#pragma once
#include "ast.hpp"
#include "result_type.hpp"
#include <string>

namespace GoldScorpion {

    Result< Program, std::string > fileToProgram( const std::string& path, bool printLex, bool printAst );

    int compile( const std::string& parseFilename, bool printLex, bool printAst );

}