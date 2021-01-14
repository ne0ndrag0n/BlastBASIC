#include "log.hpp"
#include <rang.hpp>
#include <iostream>

namespace GoldScorpion {

    void printError( const std::string& message ) {
        std::cout << rang::fgB::red << "error: " << rang::style::reset << message << std::endl;
    }

    void printSuccess( const std::string& message ) {
        std::cout << rang::fgB::green << "success: " << rang::style::reset << message << std::endl;
    }

}