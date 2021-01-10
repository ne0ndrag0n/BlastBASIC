#include "arch/m68k/md/verifier_annotation.hpp"
#include "tree_tools.hpp"
#include "error.hpp"
#include "utility.hpp"

namespace GoldScorpion::m68k::md {

    AnnotationPackage getAnnotationPackage( const AssignmentExpression& node, AnnotationSettings settings ) {
        // LHS must be an identifier
        std::optional< std::string > directive = getIdentifierName( *node.identifier );
        if( !directive ) {
            Error{ "Unable to obtain directive name for annotation", settings.nearestToken }.throwException();
        }

        switch( Utility::hash( directive->c_str() ) ) {
            case Utility::hash( "interrupt" ): {
                std::optional< std::string > value = getIdentifierName( *node.expression );
                if( !value ) {
                    Error{ "Unable to obtain value for \"interrupt\" directive: Must provide one of: \"vblank\", \"hblank\", \"external\", \"addressException\", \"illegalException\", \"divException\"", settings.nearestToken }.throwException();
                }

                switch( Utility::hash( value->c_str() ) ) {
                    case Utility::hash( "vblank" ):
                    case Utility::hash( "hblank" ):
                    case Utility::hash( "external" ):
                    case Utility::hash( "addressException" ):
                    case Utility::hash( "illegalException" ):
                    case Utility::hash( "divException" ):
                        // pass
                        return AnnotationPackage{ *directive, *value };
                    default:
                        Error{ "Invalid value \"" + *value +  "\" provided for \"interrupt\" directive: Must provide one of: \"vblank\", \"hblank\", \"external\", \"addressException\", \"illegalException\", \"divException\"", settings.nearestToken }.throwException();
                }
                break;
            }
            default:
                Error{ "Invalid annotation directive name: " + *directive, settings.nearestToken }.throwException();
        }

         return AnnotationPackage{ "", "" };
    }

    std::vector< AnnotationPackage > getAnnotationPackageList( const Annotation& annotation, AnnotationSettings settings ) {
        std::vector< AnnotationPackage > result;

        // Only Primary and AssignmentExpression valid here
        for( const auto& expression : annotation.directives ) {
            if( auto assignmentExpression = std::get_if< std::unique_ptr< AssignmentExpression > >( &expression->value ) ) {
                result.push_back( getAnnotationPackage( **assignmentExpression, settings ) );
            } else {
                Error{ "Invalid expression subtype for annotation: Valid type is AssignmentExpression", settings.nearestToken }.throwException();
            }
        }

        return result;
    }

    void checkInterrupt( const std::optional< std::string >& functionReturnType, const std::optional< Token >& nearestToken ) {
        // No interrupt can have a return type
        if( functionReturnType ) {
            Error{ "Function annotated with type \"interrupt\" cannot return any type", nearestToken }.throwException();
        }
    }

    void checkFunction( const std::string& directive, const std::optional< Token >& nearestToken, const std::optional< std::string >& functionReturnType ) {
        switch( Utility::hash( directive.c_str() ) ) {
            case Utility::hash( "interrupt" ): {
                return checkInterrupt( functionReturnType, nearestToken );
            }
            default: {
                Error{ "Internal compiler error (invalid Annotation directive type " + directive + ")", nearestToken }.throwException();
            }
        }
    }

}