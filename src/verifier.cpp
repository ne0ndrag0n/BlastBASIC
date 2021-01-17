#include "arch/m68k/md/verifier_annotation.hpp"
#include "verifier.hpp"
#include "error.hpp"
#include "type_tools.hpp"
#include "tree_tools.hpp"
#include "variant_visitor.hpp"
#include <variant>
#include <set>

namespace GoldScorpion {

    using PlatformAnnotationPackage = m68k::md::AnnotationPackage;
    using PlatformAnnotationSettings = m68k::md::AnnotationSettings;

    struct VerifierSettings {
        MemoryTracker& memory;
        std::vector< PlatformAnnotationPackage >& currentAnnotationPackage;
        std::optional< Token > nearestToken;
        std::optional< std::string > contextTypeId;
        std::optional< std::string > functionReturnType;
        bool anonymousFunctionPermitted;
        bool withinFunction;
        bool topLevelPermitted;
    };

    struct CheckedParameter {
        std::string id;
        std::string typeId;
    };

    // Forward Declarations
    static void check( const Expression& node, VerifierSettings settings );
    static void check( const Declaration& node, VerifierSettings settings );
    // end

    static std::string expectTokenString( const Token& token, const std::string& error ) {
        if( token.value ) {
            if( auto stringValue = std::get_if< std::string >( &*token.value ) ) {
                return *stringValue;
            }
        }

        Error{ error, token }.throwException();
        return "";
    }

    static long expectTokenLong( const Token& token, const std::string& error ) {
        if( token.value ) {
            if( auto longValue = std::get_if< long >( &*token.value ) ) {
                return *longValue;
            }
        }

        Error{ error, token }.throwException();
        return 0;
    }

    static void expectTokenValue( const Token& token, const std::string& error ) {
        if( !token.value ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenOfType( const Token& token, const TokenType& type, const std::string& error ) {
        if( token.type != type ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenOfType( const Token& token, const std::set< TokenType >& acceptableTypes, const std::string& error ) {
        if( !acceptableTypes.count( token.type ) ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenType( const Token& token, const std::string& error ) {
        static const std::set< TokenType > VALID_TOKENS = {
            TokenType::TOKEN_U8,
            TokenType::TOKEN_U16,
            TokenType::TOKEN_U32,
            TokenType::TOKEN_S8,
            TokenType::TOKEN_S16,
            TokenType::TOKEN_S32,
            TokenType::TOKEN_STRING,
            TokenType::TOKEN_IDENTIFIER
        };

        bool identifierNoString = ( token.type == TokenType::TOKEN_IDENTIFIER ) && ( !token.value || !std::holds_alternative< std::string >( *token.value ) );
        if( identifierNoString || !VALID_TOKENS.count( token.type ) ) {
            Error{ error, token }.throwException();
        }
    }

    static Token expectToken( const Primary& primary, std::optional< Token > nearestToken, const std::string& error ) {
        if( auto token = std::get_if< Token >( &primary.value ) ) {
            return *token;
        }

        Error{ error, nearestToken }.throwException();
        return Token{ TokenType::TOKEN_NONE, {}, 0, 0 };
    }

    static CheckedParameter checkAndExtract( const Parameter& parameter, VerifierSettings settings ) {
        expectTokenOfType( parameter.name, TokenType::TOKEN_IDENTIFIER, "Internal compiler error (Parameter identifier token not of identifier type)" );
        std::string paramName = expectTokenString( parameter.name, "Internal compiler error (Parameter identifier token contains no string alternative)" );

        // The typeId must be valid and, if a udt, declared
        expectTokenType( parameter.type.type, "Internal compiler error (Parameter type identifier not of any discernable type)" );
        std::optional< std::string > typeId = tokenToTypeId( parameter.type.type );
        if( !typeId ) {
            Error{ "Internal compiler error (Unable to obtain type id)", parameter.type.type }.throwException();
        }

        if( typeIsUdt( ValueType{ *typeId } ) && !settings.memory.findUdt( *typeId ) ) {
            Error{ "Undeclared user-defined type: " + *typeId, parameter.type.type }.throwException();
        }

        return CheckedParameter{ paramName, *typeId };
    }

    // mostly checks for internal compiler errors
    static void check( const Primary& node, VerifierSettings settings ) {
        std::visit( overloaded {

            [ &settings ]( const Token& token ) {
                switch( token.type ) {
                    case TokenType::TOKEN_THIS: {
                        if( !settings.contextTypeId ) {
                            Error{ "\"this\" specifier not permitted in this context", token }.throwException();
                        }
                        break;
                    }
                    case TokenType::TOKEN_IDENTIFIER: {
                        expectTokenValue( token, "Internal compiler error (Token of type TOKEN_IDENTIFIER has no associated value)" );
                        break;
                    }
                    case TokenType::TOKEN_LITERAL_STRING: {
                        expectTokenString( token, "Internal compiler error (Token of type TOKEN_LITERAL_STRING has no associated string)" );
                        break;
                    }
                    case TokenType::TOKEN_LITERAL_INTEGER: {
                        expectTokenLong( token, "Internal compiler error (Token of type TOKEN_LITERAL_INTEGER has no associated long)" );
                        break;
                    }
                    default:
                        // tests pass
                        break;
                }
            },

            [ &settings ]( const std::unique_ptr< Expression >& expression ) {
                check( *expression, settings );
            }

        }, node.value );
    }

    static void check( const UnaryExpression& node, VerifierSettings settings ) {
        // The only valid tokens here are "not" and "-"
        check( *node.op, settings );
        check( *node.value, settings );

        Token token = expectToken( *node.op, settings.nearestToken, "Expected: Operator of BinaryExpression to be of Token type" );
        if( !( token.type == TokenType::TOKEN_NOT || token.type == TokenType::TOKEN_MINUS ) ) {
            Error{ "Expected: \"not\" or \"-\" operator for UnaryExpression", token }.throwException();
        }
    }

    static void check( const CallExpression& node, VerifierSettings settings ) {
        // Identifier must be a callable function with a non-void return type
        check( *node.identifier, settings );
        TypeResult identifierType = getType( *node.identifier, settings.memory );

        if( !identifierType ) {
            Error{ "Unable to determine type of CallExpression identifier: " + identifierType.getError(), settings.nearestToken }.throwException();
        }

        if( !typeIsFunction( *identifierType ) ) {
            Error{ "Unable to call non-function type " + typeToString( *identifierType ), settings.nearestToken }.throwException();
        }

        // In the returned function type, we must make sure the provided argument list matches the argument list of the function
        // Only the types and the order of the types matter here
        const FunctionType& functionType = std::get< FunctionType >( *identifierType );
        if( functionType.arguments.size() != node.arguments.size() ) {
            Error{ "CallExpression requires " + std::to_string( functionType.arguments.size() ) + " arguments but " + std::to_string( node.arguments.size() ) + " arguments were provided", settings.nearestToken }.throwException();
        }

        // Iterate through function type specification and make sure types match up
        for( unsigned int i = 0; i != functionType.arguments.size(); i++ ) {
            const FunctionTypeParameter& parameter = functionType.arguments[ i ];

            check( *node.arguments[ i ], settings );
            TypeResult argumentType = getType( *node.arguments[ i ], settings.memory );
            if( !argumentType ) {
                Error{ "Unable to determine type of argument" + std::to_string( i ) + "in CallExpression: " + argumentType.getError(), settings.nearestToken }.throwException();
            }

            if( !( typesMatch( ValueType{ parameter.typeId }, *argumentType ) || integerTypesMatch( ValueType{ parameter.typeId }, *argumentType ) || coercibleToString( ValueType{ parameter.typeId }, *argumentType ) ) ) {
                Error{ "Expected: Type of argument " + std::to_string( i ) + " to be " + parameter.typeId + " but argument provided is of type " + typeToString( *argumentType ), settings.nearestToken }.throwException();
            }
        }
    }

    static void check( const BinaryExpression& node, VerifierSettings settings ) {
        // Constraints on BinaryExpressions:
        // 1) Left-hand side expression and right-hand side expression must validate
        // 2) Operator must be token-type primary and one of the following: +, -, *, /, %, or .
        // 3) For . operator:
        // - Left-hand side must return a declared UDT type
        // - Right-hand side must be a token-type primary of identifier type
        // - Right-hand side identifier must be a valid field on the left-hand side user-defined type
        // 4) For all other operators:
        // - Left-hand side type must match right-hand side type, OR
        // - Right-hand side type must be coercible to left-hand side type:
        //  - Both are integer type, or
        //  - One side is a string while another is an integer type

        check( *node.lhsValue, settings );
        check( *node.op, settings );
        check( *node.rhsValue, settings );

        Token token = expectToken( *node.op, settings.nearestToken, "Expected: Operator of BinaryExpression to be of Token type" );
        if( token.type == TokenType::TOKEN_DOT ) {
            // - Left-hand side must return a declared UDT type...
            auto lhsType = getType( *node.lhsValue, settings.memory );
            if( !lhsType || !typeIsUdt( *lhsType ) ) {
                std::string error = "Expected: Declared user-defined type as left-hand side of BinaryExpression with \".\" operator";
                if( !lhsType ) {
                    error += ": Unable to deduce type: " + lhsType.getError();
                }
                Error{ error, token }.throwException();
            }

            if( !settings.memory.findUdt( unwrapTypeId( *lhsType ) ) ) {
                Error{ "Undeclared user-defined type: " + typeToString( *lhsType ), token }.throwException();
            }

            // - Right hand side must be a token-type primary....
            if( auto primaryType = std::get_if< std::unique_ptr< Primary > >( &node.rhsValue->value ) ) {
                // ...of identifier type
                Token rhsIdentifier = expectToken( **primaryType, settings.nearestToken, "Expected: Expression of Primary token type as right-hand side of BinaryExpression with \".\" operator" );
                expectTokenOfType( rhsIdentifier, TokenType::TOKEN_IDENTIFIER, "Primary expression in RHS of BinaryExpression with \".\' operator must be of identifier type" );

                std::string rhsUdtFieldId = expectTokenString( rhsIdentifier, "Internal compiler error (BinaryExpression dot RHS token has no string alternative)" );
                if( !settings.memory.findUdtField( unwrapTypeId( *lhsType ), rhsUdtFieldId ) ) {
                    Error{ "Invalid field " + rhsUdtFieldId + " on user-defined type " + typeToString( *lhsType ), rhsIdentifier }.throwException();
                }
            } else {
                Error{ "Expected: Expression of Primary type as right-hand side of BinaryExpression with \".\" operator", settings.nearestToken }.throwException();
            }

            return;
        }

        expectTokenOfType(
            token,
            {
                TokenType::TOKEN_PLUS,
                TokenType::TOKEN_MINUS,
                TokenType::TOKEN_ASTERISK,
                TokenType::TOKEN_FORWARD_SLASH,
                TokenType::TOKEN_MODULO
            },
            "Expected: Operator of BinaryExpression to be one of \"+\",\"-\",\"*\",\"/\",\"%\",\".\""
        );

        auto lhsType = getType( *node.lhsValue, settings.memory );
        if( !lhsType ) { Error{ lhsType.getError(), settings.nearestToken }.throwException(); }

        auto rhsType = getType( *node.rhsValue, settings.memory );
        if( !rhsType ) { Error{ rhsType.getError(), settings.nearestToken }.throwException(); }

        // A type is only coercible to string if the operator is plus
        if( typeIsString( *lhsType ) || typeIsString( *rhsType ) ) {
            expectTokenOfType( token, TokenType::TOKEN_PLUS, "Expected: \"+\" operator for the concatenation of strings with string or integer types" );
        }

        // Operations cannot be performed on functions
        if( typeIsFunction( *lhsType ) || typeIsFunction( *rhsType ) ) {
            Error{ "Cannot apply BinaryExpression operation to function type " + ( typeIsFunction( *lhsType ) ? typeToString( *lhsType ) : typeToString( *rhsType ) ), settings.nearestToken }.throwException();
        }

        // Check if types are identical, and if not identical, if they can be coerced
        if( !( typesMatch( *lhsType, *rhsType ) || integerTypesMatch( *lhsType, *rhsType ) || coercibleToString( *lhsType, *rhsType ) ) ) {
            Error{ "Type mismatch: Expected type " + typeToString( *lhsType ) + " but right-hand side expression is of type " + typeToString( *rhsType ), settings.nearestToken }.throwException();
        }
    }

    static void check( const AssignmentExpression& node, VerifierSettings settings ) {
        // Begin with a simple verification of both the left-hand side and the right-hand side
        check( *node.identifier, settings );
        check( *node.expression, settings );

        // Left hand side must be either primary expression type with identifier, or binaryexpression type with dot operator
        const Expression& identifierExpression = *node.identifier;
        if( auto primaryExpression = std::get_if< std::unique_ptr< Primary > >( &identifierExpression.value ) ) {
            const Primary& primary = **primaryExpression;

            Token token = expectToken( primary, identifierExpression.nearestToken, "Expected: Primary expression in LHS of AssignmentExpression must be a single identifier" );

            // Primary expression must contain a token of type IDENTIFIER
            expectTokenOfType( token, TokenType::TOKEN_IDENTIFIER, "Primary expression in LHS of AssignmentExpression must be a single identifier" );
            expectTokenString( token, "Internal compiler error (AssignmentExpression token has no string alternative" );
        } else if( auto result = std::get_if< std::unique_ptr< BinaryExpression > >( &identifierExpression.value ) ) {
            // Validate this binary expression
            const BinaryExpression& binaryExpression = **result;

            // The above binary expression may validate as correct, but in this case, it must be a dot expression
            Token token = expectToken( *binaryExpression.op, identifierExpression.nearestToken, "BinaryExpression must have an operator of Token type" );
            expectTokenOfType( token, TokenType::TOKEN_DOT, "BinaryExpression must have operator \".\" for left-hand side of AssignmentExpression" );
        } else {
            Error{ "Invalid left-hand expression type for AssignmentExpression", settings.nearestToken }.throwException();
        }

        // Type of right hand side assignment should match type of identifier on left hand side
        auto lhsType = getType( *node.identifier, settings.memory );
        if( !lhsType ) { Error{ lhsType.getError(), settings.nearestToken }.throwException(); }

        auto rhsType = getType( *node.expression, settings.memory );
        if( !rhsType ) { Error{ rhsType.getError(), settings.nearestToken }.throwException(); }

        if( !( typesMatch( *lhsType, *rhsType ) || integerTypesMatch( *lhsType, *rhsType ) || assignmentCoercible( *lhsType, *rhsType ) ) ) {
            Error{ "Type mismatch: Expected type " + typeToString( *lhsType ) + " but expression is of type " + typeToString( *rhsType ), settings.nearestToken }.throwException();
        }
    }

    static void check( const Expression& node, VerifierSettings settings ) {
        settings.nearestToken = node.nearestToken;

        std::visit( overloaded {

            [ &settings ]( const std::unique_ptr< AssignmentExpression >& expression ) { check( *expression, settings ); },
            [ &settings ]( const std::unique_ptr< BinaryExpression >& expression ) { check( *expression, settings ); },
            [ &settings ]( const std::unique_ptr< UnaryExpression >& expression ) { check( *expression, settings ); },
            [ &settings ]( const std::unique_ptr< CallExpression >& expression ) { check( *expression, settings ); },
            [ &settings ]( const std::unique_ptr< Primary >& expression ) { check( *expression, settings ); },

        }, node.value );
    }

    static void check( const VarDeclaration& node, VerifierSettings settings ) {
        // def x as type [= value]

        // Verify x is an identifier containing a string
        expectTokenOfType( node.variable.name, TokenType::TOKEN_IDENTIFIER, "Expected: Identifier as token type for parameter declaration" );
        std::string name = expectTokenString( node.variable.name, "Internal compiler error (VarDeclaration variable.name has no string alternative)" );

        // Cannot redefine a variable in the same scope, check for this using memorytracker
        if( settings.memory.find( name, true ) ) {
            Error{ "Redeclaration of identifier " + name + " in the current scope", node.variable.name }.throwException();
        }

        // Cannot stomp over a UDT declaration
        if( settings.memory.findUdt( name ) ) {
            Error{ "Cannot use name of user-defined type " + name + " as VarDeclaration identifier", node.variable.name }.throwException();
        }

        // Verify type is either primitive or declared
        expectTokenType( node.variable.type.type, "Expected: Declared user-defined type or one of [u8, u16, u32, s8, s16, s32, string]" );

        // If type is user-defined type (IDENTIFIER) then we must verify the UDT was declared
        if( node.variable.type.type.type == TokenType::TOKEN_IDENTIFIER ) {
            std::string typeId = std::get< std::string >( *node.variable.type.type.value );
            if( !settings.memory.findUdt( typeId ) ) {
                Error{ "Undeclared user-defined type: " + typeId, node.variable.type.type }.throwException();
            }
        }

        auto identifierTitle = getIdentifierName( node.variable.name );
        if( !identifierTitle ) {
            Error{ "Internal compiler error (VarDeclaration variable.name is not an identifier)", node.variable.name }.throwException();
        }

        std::optional< std::string > typeIdConversion = tokenToTypeId( node.variable.type.type );
        if( !typeIdConversion ) {
            Error{ "Internal compiler error (VarDeclaration variable.type should be properly verified)", node.variable.type.type }.throwException();
        }

        ValueType typeId{ *typeIdConversion };

        // The type returned by the expression on the right must match the declared type, or be coercible to the type.
        if( node.value ) {
            // Validate expression
            check( **node.value, settings );

            // Get type of expression
            auto expressionType = getType( **node.value, settings.memory );
            if( !expressionType ) {
                Error{ "Internal compiler error (VarDeclaration validated Expression failed to yield a type)", {} }.throwException();
            }

            if( !( typesMatch( typeId, *expressionType ) || integerTypesMatch( typeId, *expressionType ) || assignmentCoercible( typeId, *expressionType ) ) ) {
                Error{ "Type mismatch: Expected type " + typeToString( typeId ) + " but expression is of type " + typeToString( *expressionType ), node.variable.type.type }.throwException();
            }
        }

        settings.memory.push( MemoryElement { *identifierTitle, typeId, 0, 0 } );
    }

    static void check( const ConstDeclaration& node, VerifierSettings settings ) {
        // const X as type = value

        // Verify x is an identifier containing a string
        expectTokenOfType( node.variable.name, TokenType::TOKEN_IDENTIFIER, "Expected: Identifier as token type for parameter declaration" );
        std::string name = expectTokenString( node.variable.name, "Internal compiler error (ConstDeclaration variable.name has no string alternative)" );

        // Cannot redefine a variable in the same scope, check for this using memorytracker
        if( settings.memory.find( name, true ) ) {
            Error{ "Redeclaration of identifier " + name + " in the current scope", node.variable.name }.throwException();
        }

        // Cannot stomp over a UDT declaration
        if( settings.memory.findUdt( name ) ) {
            Error{ "Cannot use name of user-defined type " + name + " as VarDeclaration identifier", node.variable.name }.throwException();
        }

        // Verify type is either primitive or declared
        expectTokenType( node.variable.type.type, "Expected: Declared user-defined type or one of [u8, u16, u32, s8, s16, s32, string]" );

        // If type is user-defined type (IDENTIFIER) then we must verify the UDT was declared
        if( node.variable.type.type.type == TokenType::TOKEN_IDENTIFIER ) {
            std::string typeId = std::get< std::string >( *node.variable.type.type.value );
            if( !settings.memory.findUdt( typeId ) ) {
                Error{ "Undeclared user-defined type: " + typeId, node.variable.type.type }.throwException();
            }
        }

        auto identifierTitle = getIdentifierName( node.variable.name );
        if( !identifierTitle ) {
            Error{ "Internal compiler error (ConstDeclaration variable.name is not an identifier)", node.variable.name }.throwException();
        }

        std::optional< std::string > typeIdConversion = tokenToTypeId( node.variable.type.type );
        if( !typeIdConversion ) {
            Error{ "Internal compiler error (ConstDeclaration variable.type should be properly verified)", node.variable.type.type }.throwException();
        }

        ValueType typeId{ *typeIdConversion };

        // Validate expression
        check( *node.value, settings );

        // Get type of expression
        auto expressionType = getType( *node.value, settings.memory );
        if( !expressionType ) {
            Error{ "Internal compiler error (ConstDeclaration validated Expression failed to yield a type)", {} }.throwException();
        }

        if( !( typesMatch( typeId, *expressionType ) || integerTypesMatch( typeId, *expressionType ) || assignmentCoercible( typeId, *expressionType ) ) ) {
            Error{ "Type mismatch: Expected type " + typeToString( typeId ) + " but expression is of type " + typeToString( *expressionType ), node.variable.type.type }.throwException();
        }

        settings.memory.insert( MemoryElement { *identifierTitle, typeId, 0, 0 }, true );
    }

    static void check( const ReturnStatement& node, VerifierSettings settings ) {
        // Return statement never valid outside function
        if( !settings.withinFunction ) {
            Error{ "Return statement not valid outside of function body", settings.nearestToken }.throwException();
        }

        // Return statement not valid if return type is specified but ReturnStatement expression is not
        if( !node.expression ) {
            if( settings.functionReturnType ) {
                Error{ "Return statement must return expression of type " + *settings.functionReturnType, settings.nearestToken }.throwException();
            }
        } else {
            // If expression is provided it must both validate and be the same type as the function
            // Return statement not valid if return type is not specified but ReturnStatement expression is
            if( !settings.functionReturnType ) {
                Error{ "Return statement must not return expression for function of void return type", settings.nearestToken }.throwException();
            }

            check( **node.expression, settings );

            auto typeId = getType( **node.expression, settings.memory );
            if( !typeId ) {
                Error{ "Internal compiler error (ReturnStatement unable to determine type for expression)", settings.nearestToken }.throwException();
            }

            ValueType functionReturnType{ *settings.functionReturnType };
            if( !( typesMatch( *typeId, functionReturnType ) || integerTypesMatch( *typeId, functionReturnType ) || assignmentCoercible( *typeId, functionReturnType ) ) ) {
                Error{ "Return statement expression of type " + typeToString( *typeId ) + " does not match function return type of " + typeToString( functionReturnType ), settings.nearestToken }.throwException();
            }
        }
    }

    static void check( const FunctionDeclaration& node, VerifierSettings settings ) {
        // Copy array instantly - it will be cleared by successive checks to the function body expressions
        std::vector< PlatformAnnotationPackage > annotationPackages = settings.currentAnnotationPackage;

        if( !node.name && !settings.anonymousFunctionPermitted ) {
            Error{ "Anonymous function declaration not permitted here", settings.nearestToken }.throwException();
        }

        std::optional< std::string > functionName;
        if( node.name ) {
            expectTokenOfType( *node.name, TokenType::TOKEN_IDENTIFIER, "Name token not of identifier type" );
            functionName = expectTokenString( *node.name, "Identifier token not of string type" );
        }

        // - No arguments can have duplicate names
        // - No arguments can refer to undeclared user-defined types
        std::set< std::string > usedNames;
        std::vector< FunctionTypeParameter > arguments;
        for( const Parameter& parameter : node.arguments ) {
            CheckedParameter checkedParameter = checkAndExtract( parameter, settings );

            if( usedNames.count( checkedParameter.id ) ) {
                Error{ "Duplicate argument identifier: " + checkedParameter.id, parameter.name }.throwException();
            } else {
                usedNames.insert( checkedParameter.id );
            }

            arguments.push_back( FunctionTypeParameter { checkedParameter.id, checkedParameter.typeId } );
        }

        // Return type must be a valid
        if( node.returnType ) {
            expectTokenType( *node.returnType, "Internal compiler error (FunctionDeclaration return type identifier not of any discernable type)" );
            if( node.returnType->type == TokenType::TOKEN_IDENTIFIER ) {
                std::string typeId = expectTokenString( *node.returnType, "Internal compiler error (FunctionDeclaration return type identifier contains no string alternative)" );
                if( !settings.memory.findUdt( typeId ) ) {
                    Error{ "Undeclared user-defined type: " + typeId, *node.returnType }.throwException();
                }
            }

            auto typeId = tokenToTypeId( *node.returnType );
            if( !typeId ) {
                Error{ "Internal compiler error (FunctionDeclaration unable to convert return type token to type id)", *node.returnType }.throwException();
            }

            settings.functionReturnType = *typeId;
        } else {
            settings.functionReturnType = {};
        }

        // All subdeclarations must validate properly - pass down settings
        settings.memory.openScope();
        // anonymousFunctionPermitted does not propagate, set within function
        settings.anonymousFunctionPermitted = true;
        settings.withinFunction = true;
        // Push arguments onto stack right-to-left so that they are in scope when subsequent checks are performed
        for( auto argument = arguments.crbegin(); argument != arguments.crend(); ++argument ) {
            // Just gotta have something in scope with the ID + type ID
            settings.memory.push( MemoryElement {
                argument->id,
                ValueType { argument->typeId },
                0,
                0
            } );
        }
        // If there is a context ID, the last variable pushed will be the "this" pointer
        if( settings.contextTypeId ) {
            settings.memory.push( MemoryElement {
                "this",
                ValueType { *settings.contextTypeId },
                0,
                0
            } );
        }
        for( const auto& declaration : node.body ) {
            check( *declaration, settings );
        }

        // If function has return type, function must contain a return
        if( settings.functionReturnType && !containsReturn( node ) ) {
            Error{ "Function has return type " + *settings.functionReturnType + " but function does not always return value of type", settings.nearestToken }.throwException();
        }

        // This should clear anything done by the function including the pushed arguments
        settings.memory.closeScope();

        // If function belongs to a type context, the validated function must be added as a field to the type
        // Otherwise, add the function to the stack
        if( settings.contextTypeId ) {
            settings.memory.addUdtField( *settings.contextTypeId, UdtField{ *functionName, FunctionType{ settings.contextTypeId, arguments, settings.functionReturnType } } );
        } else {
            settings.memory.push( MemoryElement{ functionName, FunctionType{ {}, arguments, settings.functionReturnType }, 0, 0 } );
        }

        // After function is fully-validated, check annotations that may be attached
        for( const PlatformAnnotationPackage& package : annotationPackages ) {
            m68k::md::checkFunction( package.id, settings.nearestToken, settings.functionReturnType );
        }
    }

    static void check( const TypeDeclaration& node, VerifierSettings settings ) {
        // Type name is a single token of string type
        expectTokenOfType( node.name, TokenType::TOKEN_IDENTIFIER, "Internal compiler error (TypeDeclaration token not of identifier type)" );
        std::string typeId = expectTokenString( node.name, "Internal compiler error (TypeDeclaration token of identifier type contains no string alternative)" );

        // Typeid must not already exist in the current scope
        if( settings.memory.findUdt( typeId, true ) ) {
            Error{ "Redeclaration of user-defined type " + typeId + " in the current scope", node.name }.throwException();
        }

        // Cannot stomp an existing VarDeclaration
        if( settings.memory.find( typeId ) ) {
            Error{ "Cannot use name of existing variable " + typeId + " as TypeDeclaration identifier", node.name }.throwException();
        }

        // Each parameter must contain an identifier/string token and either a primitive type or a declared user-defined type
        // No two fields may have the same name
        std::set< std::string > declaredNames;
        std::vector< UdtField > fields;
        for( const Parameter& parameter : node.fields ) {
            CheckedParameter checkedParameter = checkAndExtract( parameter, settings );

            if( declaredNames.count( checkedParameter.id ) ) {
                Error{ "Redeclaration of user-defined type field: " + checkedParameter.id, parameter.name }.throwException();
            } else {
                declaredNames.insert( checkedParameter.id );
            }

            // Push onto the fields array
            fields.push_back( UdtField { checkedParameter.id, ValueType { checkedParameter.typeId } } );
        }

        // User-defined type must contain at least one field
        if( fields.empty() ) {
            Error{ "User-defined type must declare at least one field", node.name }.throwException();
        }

        // For all functions inside this type, set the contextTypeId so that "this" tokens may have obtainable types
        settings.contextTypeId = typeId;

        // Add the user-defined type to the memory tracker
        settings.memory.addUdt( UserDefinedType { typeId, fields } );

        // Check all member functions
        for( const std::unique_ptr< FunctionDeclaration >& function : node.functions ) {
            check( *function, settings );
        }
    }

    static void check( const Annotation& node, VerifierSettings settings ) {
        // Must be at least one directive
        if( node.directives.empty() ) {
            Error{ "Must provide at least one expression for annotation", settings.nearestToken }.throwException();
        }

        // Attempt to get a seties of annotation packages
        // The package will be consumed by the next declaration
        settings.currentAnnotationPackage = getAnnotationPackageList( node, PlatformAnnotationSettings{ settings.memory, settings.nearestToken } );
    }

    static void check( const ImportDeclaration& node, VerifierSettings settings ) {
        // ImportDeclaration only valid at top level declaration
        if( !settings.topLevelPermitted ) {
            Error{ "ImportDeclaration only valid at top level of file", settings.nearestToken }.throwException();
        }

        if( node.path == "" || node.path.empty() ) {
            Error{ "ImportDeclaration must contain valid path", settings.nearestToken }.throwException();
        }

        // Actual path will be validated by the file function
    }

    static void check( const Statement& node, VerifierSettings settings ) {
        settings.nearestToken = node.nearestToken;

        std::visit( overloaded {

            [ &settings ]( const std::unique_ptr< ExpressionStatement >& statement ) { check( *(statement->value), settings ); },
			[]( const std::unique_ptr< ForStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype ForStatement)", {} }.throwException(); },
			[]( const std::unique_ptr< IfStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype IfStatement)", {} }.throwException(); },
			[ &settings ]( const std::unique_ptr< ReturnStatement >& statement ) { check( *statement, settings ); },
			[]( const std::unique_ptr< AsmStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype AsmStatement)", {} }.throwException(); },
			[]( const std::unique_ptr< WhileStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype WhileStatement)", {} }.throwException(); }

        }, node.value );
    }

    static void check( const Declaration& node, VerifierSettings settings ) {
        settings.nearestToken = node.nearestToken;
        bool freshAnnotation = false;

        if( !std::holds_alternative< std::unique_ptr< ImportDeclaration > >( node.value ) ) {
            settings.topLevelPermitted = false;
        }

        std::visit( overloaded {

            [ &settings, &freshAnnotation ]( const std::unique_ptr< Annotation >& declaration ) { check( *declaration, settings ); freshAnnotation = true; },
            [ &settings ]( const std::unique_ptr< VarDeclaration >& declaration ) { check( *declaration, settings ); },
            [ &settings ]( const std::unique_ptr< ConstDeclaration >& declaration ) { check( *declaration, settings ); },
            [ &settings ]( const std::unique_ptr< FunctionDeclaration >& declaration ) { check( *declaration, settings ); },
            [ &settings ]( const std::unique_ptr< TypeDeclaration >& declaration ) { check( *declaration, settings ); },
            [ &settings ]( const std::unique_ptr< ImportDeclaration >& declaration ) { check( *declaration, settings ); },
            [ &settings ]( const std::unique_ptr< Statement >& declaration ) { check( *declaration, settings ); }

        }, node.value );

        // PlatformAnnotationPackages are cleared if they are not consumed and they are not brand new
        if( !freshAnnotation ) {
            settings.currentAnnotationPackage.clear();
        }
    }

    /**
     * Run a verification step to make sure items are logically consistent
     */
    std::optional< std::string > check( const Program& program ) {
        MemoryTracker memory;
        std::vector< PlatformAnnotationPackage > currentAnnotationPackage;

        VerifierSettings settings{ memory, currentAnnotationPackage, {}, {}, {}, false, false, true };
        for( const auto& declaration : program.statements ) {
            try {
                check( *declaration, settings );
            } catch( std::runtime_error e ) {
                return e.what();
            }
        }

        return {};
    }

}