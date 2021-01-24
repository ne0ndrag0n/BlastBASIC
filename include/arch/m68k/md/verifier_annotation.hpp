#pragma once
#include "ast.hpp"
#include "memory_tracker.hpp"
#include "token.hpp"
#include "symbol.hpp"
#include <variant>
#include <string>
#include <vector>

namespace GoldScorpion::m68k::md {

    // Valid annotations for m68k-md target:
    //
    // `interrupt` (interrupt=[IDENTIFIER])
    // Adjacent nodes: FunctionDeclaration (not belonging to TypeDeclaration)
    // Defines the function below as the target for an MD interrupt.
    // Valid interrupt targets are:
    // "vblank", "hblank", "external", "addressException", "illegalException", "divException"

    using AnnotationValue = std::variant< std::string, long, bool >;

    struct AnnotationPackage {
        std::string id;
        AnnotationValue value;
    };

    struct AnnotationSettings {
        SymbolResolver& symbols;
        std::optional< Token > nearestToken;
    };

    AnnotationPackage getAnnotationPackage( const AssignmentExpression& node, AnnotationSettings settings );

    std::vector< AnnotationPackage > getAnnotationPackageList( const Annotation& annotation, AnnotationSettings settings );

    void checkInterrupt( const std::optional< SymbolType >& functionReturnType, const std::optional< Token >& nearestToken );

    void checkFunction( const std::string& directive, const std::optional< Token >& nearestToken, const std::optional< SymbolType >& functionReturnType );

}