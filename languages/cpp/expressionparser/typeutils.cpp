/* This file is part of KDevelop
    Copyright (C) 2007 David Nolden [david.nolden.kdevelop  art-master.de]

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "typeutils.h"
#include "duchainbuilder/cpptypes.h"
#include "duchain/ducontext.h"
#include "duchain/classfunctiondeclaration.h"

namespace TypeUtils {
  using namespace KDevelop;
  
  AbstractType* realType(AbstractType* base, bool* constant) {
    
    CppReferenceType* ref = dynamic_cast<CppReferenceType*>( base );
    
    while( ref ) {
      if( constant )
        (*constant) |= ref->isConstant();
      base = ref->baseType().data();
      ref = dynamic_cast<CppReferenceType*>( base );
    }

    return base;
  }
  
  AbstractType* targetType(AbstractType* base, bool* constant) {
    
    CppReferenceType* ref = dynamic_cast<CppReferenceType*>( base );
    CppPointerType* pnt = dynamic_cast<CppPointerType*>( base );
    
    while( ref || pnt ) {
      if( ref ) {
        if( constant )
          (*constant) |= ref->isConstant();
        base = ref->baseType().data();
      } else {
        if( constant )
          (*constant) |= pnt->isConstant();
        base = pnt->baseType().data();
      }
      ref = dynamic_cast<CppReferenceType*>( base );
      pnt = dynamic_cast<CppPointerType*>( base );
    }

    return base;
  }
  
  bool isPointerType(AbstractType* type) {
    return dynamic_cast<PointerType*>( realType(type) );
  }
  
  bool isReferenceType(AbstractType* type) {
    return dynamic_cast<ReferenceType*>( type );
  }
  
  bool isConstant( AbstractType* t ) {
    CppCVType* cv = dynamic_cast<CppCVType*>( t );
    return cv && cv->isConstant();
  }

  bool isNullType( AbstractType* t ) {
    ///@todo implement
#warning implement
    return false;
  }

    const int unsignedIntConversionRank = 4;

  int integerConversionRank( CppIntegralType* type ) {
    /** 
     * Ranks:
     * 1 - bool
     * 2 - 1 byte, char
     * 3 - 2 byte,  short int, wchar_t, unsigned short int
     * 4 - 4 byte,  int, unsigned int
     * 5 - 4 byte,  long int
     * 6 - 4 byte, long long int
     **/
    switch( type->integralType() ) {
      case CppIntegralType::TypeBool:
        return 1;
      break;
      case CppIntegralType::TypeChar:
        return 2;
      break;
      case CppIntegralType::TypeWchar_t:
        return 3;
      break;
      case CppIntegralType::TypeInt:
        if( type->typeModifiers() & CppIntegralType::ModifierShort )
          return 3;
        if( type->typeModifiers() & CppIntegralType::ModifierLong )
          return 5;
        if( type->typeModifiers() & CppIntegralType::ModifierLongLong )
          return 6;

        return 4; //default-integer
      break;
      //All other types have no integer-conversion-rank
    };
    return 0;
  }
  bool isIntegerType( CppIntegralType* type ) {
    return integerConversionRank(type) != 0; //integerConversionRank returns 0 for non-integer types
  }

  bool isFloatingPointType( CppIntegralType* type ) {
    return type->integralType() == CppIntegralType::TypeFloat || type->integralType() == CppIntegralType::TypeDouble;
  }

  bool isVoidType( AbstractType* type ) {
    CppIntegralType* integral = dynamic_cast<CppIntegralType*>(type);
    if( !integral ) return false;
    return integral->integralType() == CppIntegralType::TypeVoid;
  }
  
  ///Returns whether base is a base-class of c
  bool isPublicBaseClass( const CppClassType* c, CppClassType* base ) {
    foreach( const CppClassType::BaseClassInstance& b, c->baseClasses() ) {
      if( b.access != KDevelop::Declaration::Private ) {
        if( b.baseClass.data() == base )
          return true;
        if( isPublicBaseClass( b.baseClass.data(), base ) )
          return true;
      }
    }
    return false;
  }

  DUContext* getInternalContext( Declaration* declaration ) {
    IdentifiedType* idType = dynamic_cast<IdentifiedType*>(declaration->abstractType().data());
    QList<DUContext*> internalContexts;
    internalContexts = declaration->context()->findContexts(DUContext::Class, idType->identifier());
    internalContexts += declaration->context()->findContexts(DUContext::Namespace, idType->identifier());
    internalContexts += declaration->context()->findContexts(DUContext::Global, idType->identifier());
    if( internalContexts.isEmpty() )
      return 0;
    else
      return internalContexts.front();
  }
    
  void getConversionFunctions(CppClassType* klass, QHash<AbstractType*, Declaration*>& functions, bool mustBeConstant)  {
    DUContext* context = getInternalContext( klass->declaration() );

    QList<Declaration*> declarations = context->localDeclarations();
    for( QList<Declaration*>::iterator it = declarations.begin(); it != declarations.end(); ++it ) {
      if( (*it)->abstractType() ) {
        CppFunctionType* function = dynamic_cast<CppFunctionType*>( (*it)->abstractType().data() );
        ClassFunctionDeclaration* functionDeclaration = dynamic_cast<ClassFunctionDeclaration*>( *it );
        Q_ASSERT(functionDeclaration);
        if( functionDeclaration->isConversionFunction() && !functions.contains(function->returnType().data()) && (!mustBeConstant || function->isConstant()) ) {
          functions[function->returnType().data()] =  *it;
        }
      }
    }

    for( QList<CppClassType::BaseClassInstance>::const_iterator it =  klass->baseClasses().begin(); it != klass->baseClasses().end(); ++it ) {
      if( (*it).access != KDevelop::Declaration::Private )
        getConversionFunctions( const_cast<CppClassType::BaseClassInstance&>((*it)).baseClass.data(), functions );//we need const-cast here because the constant list makes also the pointers constant, which is not intended
    }
  }

  void getConstructors(CppClassType* klass, QList<Declaration*>& functions) {
    DUContext* context = getInternalContext( klass->declaration() );
    
    QList<Declaration*> declarations = context->localDeclarations();
    for( QList<Declaration*>::iterator it = declarations.begin(); it != declarations.end(); ++it ) {
      ClassFunctionDeclaration* functionDeclaration = dynamic_cast<ClassFunctionDeclaration*>( *it );
      if( functionDeclaration && functionDeclaration->isConstructor() )
        functions <<  *it;
    }
  }
}
