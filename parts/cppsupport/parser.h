/***************************************************************************
 *   Copyright (C) 2002 by Roberto Raggi                                   *
 *   roberto@kdevelop.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qvaluestack.h>

class ParserPrivateData;

class Driver;
class Lexer;
class Token;
class Error;

class Parser
{
public:
    Parser( Driver* driver, Lexer* lexer );
    virtual ~Parser();

private:
    virtual bool reportError( const Error& err );
    /*TODO: remove*/ virtual bool reportError( const QString& msg );
    /*TODO: remove*/ virtual void syntaxError();
    /*TODO: remove*/ virtual void parseError();

public /*rules*/ :

    bool parseTranslationUnit( TranslationUnitAST::Node& node );

    bool parseDefinition( DeclarationAST::Node& node );
    bool parseBlockDeclaration( DeclarationAST::Node& node );
    bool parseLinkageSpecification( DeclarationAST::Node& node );
    bool parseLinkageBody( LinkageBodyAST::Node& node );
    bool parseNamespace( DeclarationAST::Node& node );
    bool parseNamespaceAliasDefinition( DeclarationAST::Node& node );
    bool parseUsing( DeclarationAST::Node& node );
    bool parseUsingDirective( DeclarationAST::Node& node );
    bool parseTypedef( DeclarationAST::Node& node );
    bool parseAsmDefinition( DeclarationAST::Node& node );
    bool parseTemplateDeclaration( DeclarationAST::Node& node );
    bool parseDeclaration( DeclarationAST::Node& node );
    
    bool parseNestedNameSpecifier( NestedNameSpecifierAST::Node& node );
    bool parseUnqualifiedName( AST::Node& node );
    bool parseStringLiteral( AST::Node& node );
    bool parseName( NameAST::Node& node );
    bool parseOperatorFunctionId( AST::Node& node );
    bool parseTemplateArgumentList( TemplateArgumentListAST::Node& node );
    bool parseOperator( AST::Node& node );
    bool parseCvQualify( GroupAST::Node& node );
    bool parseSimpleTypeSpecifier( TypeSpecifierAST::Node& node );
    bool parsePtrOperator( AST::Node& node );
    bool parseTemplateArgument( AST::Node& node );
    bool parseTypeSpecifier( TypeSpecifierAST::Node& node );
    bool parseTypeSpecifierOrClassSpec( TypeSpecifierAST::Node& node );
    bool parseDeclarator( DeclaratorAST::Node& node );
    bool parseTemplateParameterList( AST::Node& node );
    bool parseTemplateParameter( AST::Node& node );
    bool parseStorageClassSpecifier( GroupAST::Node& node );
    bool parseFunctionSpecifier( GroupAST::Node& node );
    bool parseInitDeclaratorList( InitDeclaratorListAST::Node& node );
    bool parseInitDeclarator( InitDeclaratorAST::Node& node );
    bool parseParameterDeclarationClause( ParameterDeclarationClauseAST::Node& node );
    bool parseCtorInitializer( AST::Node& node );
    bool parsePtrToMember( AST::Node& node );
    bool parseEnumSpecifier( TypeSpecifierAST::Node& node );
    bool parseClassSpecifier( TypeSpecifierAST::Node& node );
    bool parseElaboratedTypeSpecifier( TypeSpecifierAST::Node& node );
    bool parseDeclaratorId( NameAST::Node& node );
    bool parseExceptionSpecification( AST::Node& node );
    bool parseEnumerator( EnumeratorAST::Node& node );
    bool parseTypeParameter( AST::Node& node );
    bool parseParameterDeclaration( ParameterDeclarationAST::Node& node );
    bool parseTypeId( AST::Node& node );
    bool parseAbstractDeclarator( DeclaratorAST::Node& node );
    bool parseParameterDeclarationList( ParameterDeclarationListAST::Node& node );
    bool parseMemberSpecification( DeclarationAST::Node& node );
    bool parseAccessSpecifier( AST::Node& node );
    bool parseTypeIdList( AST::Node& node );
    bool parseMemInitializerList( AST::Node& node );
    bool parseMemInitializer( AST::Node& node );
    bool parseInitializer( AST::Node& node );
    bool parseBaseClause( BaseClauseAST::Node& node );
    bool parseBaseSpecifier( BaseSpecifierAST::Node& node );
    bool parseInitializerClause( AST::Node& node );
    bool parseMemInitializerId( NameAST::Node& node );
    bool parseFunctionBody( StatementListAST::Node& node );

    // expression
    bool skipExpression( AST::Node& node );
    bool skipConstantExpression( AST::Node& node );
    bool skipCommaExpression( AST::Node& node );
    bool skipAssignmentExpression( AST::Node& node );
    bool skipExpressionStatement( StatementAST::Node& node );

#if 0
    bool parseExpression( AST::Node& node );
    bool parsePrimaryExpression( AST::Node& node );
    bool parsePostfixExpression( AST::Node& node );
    bool parseUnaryExpression( AST::Node& node );
    bool parseNewExpression( AST::Node& node );
    bool parseNewTypeId( AST::Node& node );
    bool parseNewDeclarator( AST::Node& node );
    bool parseNewInitializer( AST::Node& node );
    bool parseDeleteExpression( AST::Node& node );
    bool parseCastExpression( AST::Node& node );
    bool parsePmExpression( AST::Node& node );
    bool parseMultiplicativeExpression( AST::Node& node );
    bool parseAdditiveExpression( AST::Node& node );
    bool parseShiftExpression( AST::Node& node );
    bool parseRelationalExpression( AST::Node& node );
    bool parseEqualityExpression( AST::Node& node );
    bool parseAndExpression( AST::Node& node );
    bool parseExclusiveOrExpression( AST::Node& node );
    bool parseInclusiveOrExpression( AST::Node& node );
    bool parseLogicalAndExpression( AST::Node& node );
    bool parseLogicalOrExpression( AST::Node& node );
    bool parseConditionalExpression( AST::Node& node );
    bool parseAssignmentExpression( AST::Node& node );
    bool parseConstantExpression( AST::Node& node );
    bool parseCommaExpression( AST::Node& node );
    bool parseThrowExpression( AST::Node& node );
#endif

    // statement
    bool parseCondition( AST::Node& node );
    bool parseStatement( StatementAST::Node& node );
    bool parseWhileStatement( StatementAST::Node& node );
    bool parseDoStatement( StatementAST::Node& node );
    bool parseForStatement( StatementAST::Node& node );
    bool parseCompoundStatement( StatementAST::Node& node );
    bool parseForInitStatement( StatementAST::Node& node );
    bool parseIfStatement( StatementAST::Node& node );
    bool parseSwitchStatement( StatementAST::Node& node );
    bool parseLabeledStatement( StatementAST::Node& node );
    bool parseDeclarationStatement( StatementAST::Node& node );
    bool parseTryBlockStatement( StatementAST::Node& node );

    bool skipUntil( int token );
    bool skipUntilDeclaration();
    bool skipUntilStatement();
    bool skip( int l, int r );
    QString toString( int start, int end, const QString& sep=" " ) const;

private:
    ParserPrivateData* d;
    Driver* m_driver;
    Lexer* lex;
    int m_problems;
    int m_maxProblems;
};


#endif
