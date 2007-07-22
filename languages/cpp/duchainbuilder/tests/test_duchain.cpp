/* This file is part of KDevelop
    Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

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

#include "test_duchain.h"

#include <QtTest/QtTest>

#include <duchain.h>
#include <duchainlock.h>
#include <topducontext.h>
#include "declarationbuilder.h"
#include "usebuilder.h"
#include <declaration.h>
#include "cpptypes.h"
#include "templateparameterdeclaration.h"
#include <documentrange.h>
#include "cppeditorintegrator.h"
#include "dumptypes.h"
#include "environmentmanager.h"
#include "hashedstring.h"
#include "typeutils.h"
#include "templatedeclaration.h"

#include "tokens.h"
#include "parsesession.h"
#include <symboltable.h>

#include "rpp/preprocessor.h"

using namespace KTextEditor;
using namespace TypeUtils;
using namespace KDevelop;
using namespace Cpp;

QTEST_MAIN(TestDUChain)

namespace QTest {
  template<>
  char* toString(const Cursor& cursor)
  {
    QByteArray ba = "Cursor(";
    ba += QByteArray::number(cursor.line()) + ", " + QByteArray::number(cursor.column());
    ba += ')';
    return qstrdup(ba.data());
  }
  template<>
  char* toString(const QualifiedIdentifier& id)
  {
    QByteArray arr = id.toString().toLatin1();
    return qstrdup(arr.data());
  }
  template<>
  char* toString(const Identifier& id)
  {
    QByteArray arr = id.toString().toLatin1();
    return qstrdup(arr.data());
  }
  /*template<>
  char* toString(QualifiedIdentifier::MatchTypes t)
  {
    QString ret;
    switch (t) {
      case QualifiedIdentifier::NoMatch:
        ret = "No Match";
        break;
      case QualifiedIdentifier::Contains:
        ret = "Contains";
        break;
      case QualifiedIdentifier::ContainedBy:
        ret = "Contained By";
        break;
      case QualifiedIdentifier::ExactMatch:
        ret = "Exact Match";
        break;
    }
    QByteArray arr = ret.toString().toLatin1();
    return qstrdup(arr.data());
  }*/
  template<>
  char* toString(const Declaration& def)
  {
    QString s = QString("Declaration %1 (%2): %3").arg(def.identifier().toString()).arg(def.qualifiedIdentifier().toString()).arg(reinterpret_cast<long>(&def));
    return qstrdup(s.toLatin1().constData());
  }
  template<>
  char* toString(const KSharedPtr<AbstractType>& type)
  {
    QString s = QString("Type: %1").arg(type ? type->toString() : QString("<null>"));
    return qstrdup(s.toLatin1().constData());
  }
}

Declaration* getDeclaration( AbstractType::Ptr base ) {
  if( !base ) return 0;

  IdentifiedType* idType = dynamic_cast<IdentifiedType*>(base.data());
  if( idType ) {
    return idType->declaration();
  } else {
    return 0;
  }
}


#define TEST_FILE_PARSE_ONLY if (testFileParseOnly) QSKIP("Skip", SkipSingle);
TestDUChain::TestDUChain()
{
  testFileParseOnly = false;
}

void TestDUChain::initTestCase()
{
  noDef = 0;

  file1 = "file:///media/data/kdedev/4.0/kdevelop/languages/cpp/parser/duchain.cpp";
  file2 = "file:///media/data/kdedev/4.0/kdevelop/languages/cpp/parser/dubuilder.cpp";

  topContext = new TopDUContext(new KDevelop::DocumentRange(file1, Range(0,0,25,0)));
  DUChainWriteLocker lock(DUChain::lock());
  
  DUChain::self()->addDocumentChain(IdentifiedFile(file1), topContext);

  typeVoid = AbstractType::Ptr::staticCast(TypeRepository::self()->integral(CppIntegralType::TypeVoid));
  typeInt = AbstractType::Ptr::staticCast(TypeRepository::self()->integral(CppIntegralType::TypeInt));
}

void TestDUChain::cleanupTestCase()
{
  /*delete type1;
  delete type2;
  delete type3;*/

  DUChainWriteLocker lock(DUChain::lock());

  KDevelop::EditorIntegrator::releaseTopRange(topContext->textRangePtr());
  delete topContext;
}

Declaration* TestDUChain::findDeclaration(DUContext* context, const Identifier& id, const Cursor& position)
{
  QList<Declaration*> ret = context->findDeclarations(id, position);
  if (ret.count())
    return ret.first();
  return 0;
}

Declaration* TestDUChain::findDeclaration(DUContext* context, const QualifiedIdentifier& id, const Cursor& position)
{
  QList<Declaration*> ret = context->findDeclarations(id, position);
  if (ret.count())
    return ret.first();
  return 0;
}

void TestDUChain::testIdentifiers()
{
  TEST_FILE_PARSE_ONLY

  QualifiedIdentifier aj("::Area::jump");
  QCOMPARE(aj.count(), 2);
  QCOMPARE(aj.explicitlyGlobal(), true);
  QCOMPARE(aj.at(0), Identifier("Area"));
  QCOMPARE(aj.at(1), Identifier("jump"));

  QualifiedIdentifier aj2 = QualifiedIdentifier("Area::jump");
  QCOMPARE(aj2.count(), 2);
  QCOMPARE(aj2.explicitlyGlobal(), false);
  QCOMPARE(aj2.at(0), Identifier("Area"));
  QCOMPARE(aj2.at(1), Identifier("jump"));

  QCOMPARE(aj == aj2, true);

  QCOMPARE(aj.match(aj2), QualifiedIdentifier::ExactMatch);

  QualifiedIdentifier ajt("Area::jump::test");
  QualifiedIdentifier jt("jump::test");
  QualifiedIdentifier ajt2("Area::jump::tes");

  QCOMPARE(aj2.match(ajt), QualifiedIdentifier::NoMatch);
  QCOMPARE(ajt.match(aj2), QualifiedIdentifier::NoMatch);
  //QCOMPARE(jt.match(aj2), QualifiedIdentifier::ContainedBy); ///@todo reenable this(but it fails)
  QCOMPARE(ajt.match(jt), QualifiedIdentifier::Contains);

  QCOMPARE(aj2.match(ajt2), QualifiedIdentifier::NoMatch);
  QCOMPARE(ajt2.match(aj2), QualifiedIdentifier::NoMatch);
}

void TestDUChain::testContextRelationships()
{
  TEST_FILE_PARSE_ONLY

  QCOMPARE(DUChain::self()->chainForDocument(file1), topContext);

  DUChainWriteLocker lock(DUChain::lock());

  DUContext* firstChild = new DUContext(new KDevelop::DocumentRange(file1, Range(4,4, 10,3)), topContext);

  QCOMPARE(firstChild->parentContext(), topContext);
  QCOMPARE(firstChild->childContexts().count(), 0);
  QCOMPARE(topContext->childContexts().count(), 1);
  QCOMPARE(topContext->childContexts().last(), firstChild);

  DUContext* secondChild = new DUContext(new KDevelop::DocumentRange(file1, Range(14,4, 19,3)), topContext);

  QCOMPARE(topContext->childContexts().count(), 2);
  QCOMPARE(topContext->childContexts()[1], secondChild);

  DUContext* thirdChild = new DUContext(new KDevelop::DocumentRange(file1, Range(10,4, 14,3)), topContext);

  QCOMPARE(topContext->childContexts().count(), 3);
  QCOMPARE(topContext->childContexts()[1], thirdChild);

  topContext->deleteChildContextsRecursively();
  QVERIFY(topContext->childContexts().isEmpty());
}

void TestDUChain::testDeclareInt()
{
  TEST_FILE_PARSE_ONLY

  QByteArray method("int i;");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 0);
  QCOMPARE(top->localDeclarations().count(), 1);
  QVERIFY(top->localScopeIdentifier().isEmpty());

  Declaration* def = top->localDeclarations().first();
  QCOMPARE(def->identifier(), Identifier("i"));
  QCOMPARE(findDeclaration(top, def->identifier()), def);

  release(top);
}

void TestDUChain::testIntegralTypes()
{
  TEST_FILE_PARSE_ONLY

  QByteArray method("const unsigned int i, k; volatile long double j; int* l; double * const * m; const int& n = l;");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 0);
  QCOMPARE(top->localDeclarations().count(), 6);
  QVERIFY(top->localScopeIdentifier().isEmpty());

  Declaration* defI = top->localDeclarations().first();
  QCOMPARE(defI->identifier(), Identifier("i"));
  QCOMPARE(findDeclaration(top, defI->identifier()), defI);
  QVERIFY(defI->type<CppIntegralType>());
  QCOMPARE(defI->type<CppIntegralType>()->integralType(), CppIntegralType::TypeInt);
  QCOMPARE(defI->type<CppIntegralType>()->typeModifiers(), CppIntegralType::ModifierUnsigned);
  QVERIFY(defI->type<CppIntegralType>()->isConstant());
  QVERIFY(!defI->type<CppIntegralType>()->isVolatile());

  Declaration* defK = top->localDeclarations()[1];
  QCOMPARE(defK->identifier(), Identifier("k"));
  QCOMPARE(defK->type<CppIntegralType>(), defI->type<CppIntegralType>());

  Declaration* defJ = top->localDeclarations()[2];
  QCOMPARE(defJ->identifier(), Identifier("j"));
  QCOMPARE(findDeclaration(top, defJ->identifier()), defJ);
  QVERIFY(defJ->type<CppIntegralType>());
  QCOMPARE(defJ->type<CppIntegralType>()->integralType(), CppIntegralType::TypeDouble);
  QCOMPARE(defJ->type<CppIntegralType>()->typeModifiers(), CppIntegralType::ModifierLong);
  QVERIFY(!defJ->type<CppIntegralType>()->isConstant());
  QVERIFY(defJ->type<CppIntegralType>()->isVolatile());

  Declaration* defL = top->localDeclarations()[3];
  QCOMPARE(defL->identifier(), Identifier("l"));
  QVERIFY(defL->type<CppPointerType>());
  QCOMPARE(defL->type<CppPointerType>()->baseType(), typeInt);
  QVERIFY(!defL->type<CppPointerType>()->isConstant());
  QVERIFY(!defL->type<CppPointerType>()->isVolatile());

  Declaration* defM = top->localDeclarations()[4];
  QCOMPARE(defM->identifier(), Identifier("m"));
  CppPointerType::Ptr firstpointer = defM->type<CppPointerType>();
  QVERIFY(firstpointer);
  QVERIFY(!firstpointer->isConstant());
  QVERIFY(!firstpointer->isVolatile());
  CppPointerType::Ptr secondpointer = CppPointerType::Ptr::dynamicCast(firstpointer->baseType());
  QVERIFY(secondpointer);
  QVERIFY(secondpointer->isConstant());
  QVERIFY(!secondpointer->isVolatile());
  CppIntegralType::Ptr base = CppIntegralType::Ptr::dynamicCast(secondpointer->baseType());
  QVERIFY(base);
  QCOMPARE(base->integralType(), CppIntegralType::TypeDouble);
  QVERIFY(!base->isConstant());
  QVERIFY(!base->isVolatile());

  Declaration* defN = top->localDeclarations()[5];
  QCOMPARE(defN->identifier(), Identifier("n"));
  QVERIFY(defN->type<CppReferenceType>());
  base = CppIntegralType::Ptr::dynamicCast(defN->type<CppReferenceType>()->baseType());
  QVERIFY(base);
  QCOMPARE(base->integralType(), CppIntegralType::TypeInt);
  QVERIFY(base->isConstant());
  QVERIFY(!base->isVolatile());

  release(top);
}

void TestDUChain::testArrayType()
{
  TEST_FILE_PARSE_ONLY

  QByteArray method("int i[3];");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 0);
  QCOMPARE(top->localDeclarations().count(), 1);
  QVERIFY(top->localScopeIdentifier().isEmpty());

  Declaration* defI = top->localDeclarations().first();
  QCOMPARE(defI->identifier(), Identifier("i"));
  QCOMPARE(findDeclaration(top, defI->identifier()), defI);

  ArrayType::Ptr array = defI->type<ArrayType>();
  QVERIFY(array);
  CppIntegralType::Ptr element = CppIntegralType::Ptr::dynamicCast(array->elementType());
  QVERIFY(element);
  QCOMPARE(element->integralType(), CppIntegralType::TypeInt);
  QCOMPARE(array->dimension(), 3);

  release(top);
}

void TestDUChain::testDeclareFor()
{
  TEST_FILE_PARSE_ONLY

  //                 0         1         2         3         4         5
  //                 012345678901234567890123456789012345678901234567890123456789
  QByteArray method("int main() { for (int i = 0; i < 10; i++) { if (i == 4) return; } }");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 2);
  QCOMPARE(top->localDeclarations().count(), 1);
  QVERIFY(top->localScopeIdentifier().isEmpty());

  Declaration* defMain = top->localDeclarations().first();
  QCOMPARE(defMain->identifier(), Identifier("main"));
  QCOMPARE(findDeclaration(top, defMain->identifier()), defMain);
  QVERIFY(defMain->type<CppFunctionType>());
  QCOMPARE(defMain->type<CppFunctionType>()->returnType(), typeInt);
  QCOMPARE(defMain->type<CppFunctionType>()->arguments().count(), 0);
  QVERIFY(!defMain->type<CppFunctionType>()->isConstant());
  QVERIFY(!defMain->type<CppFunctionType>()->isVolatile());

  QCOMPARE(findDeclaration(top, Identifier("i")), noDef);

  DUContext* main = top->childContexts()[1];
  QVERIFY(main->parentContext());
  QCOMPARE(main->importedParentContexts().count(), 1);
  QCOMPARE(main->childContexts().count(), 2);
  QCOMPARE(main->localDeclarations().count(), 0);
  QVERIFY(main->localScopeIdentifier().isEmpty());

  QCOMPARE(findDeclaration(main, Identifier("i")), noDef);

  DUContext* forCtx = main->childContexts()[1];
  QVERIFY(forCtx->parentContext());
  QCOMPARE(forCtx->importedParentContexts().count(), 1);
  QCOMPARE(forCtx->childContexts().count(), 2);
  QCOMPARE(forCtx->localDeclarations().count(), 0);
  QVERIFY(forCtx->localScopeIdentifier().isEmpty());

  DUContext* forParamCtx = forCtx->importedParentContexts().first();
  QVERIFY(forParamCtx->parentContext());
  QCOMPARE(forParamCtx->importedParentContexts().count(), 0);
  QCOMPARE(forParamCtx->childContexts().count(), 0);
  QCOMPARE(forParamCtx->localDeclarations().count(), 1);
  QVERIFY(forParamCtx->localScopeIdentifier().isEmpty());

  Declaration* defI = forParamCtx->localDeclarations().first();
  QCOMPARE(defI->identifier(), Identifier("i"));
  QCOMPARE(defI->uses().count(), 3);

  QCOMPARE(findDeclaration(forCtx, defI->identifier()), defI);

  DUContext* ifCtx = forCtx->childContexts()[1];
  QVERIFY(ifCtx->parentContext());
  QCOMPARE(ifCtx->importedParentContexts().count(), 1);
  QCOMPARE(ifCtx->childContexts().count(), 0);
  QCOMPARE(ifCtx->localDeclarations().count(), 0);
  QVERIFY(ifCtx->localScopeIdentifier().isEmpty());

  QCOMPARE(findDeclaration(ifCtx,  defI->identifier()), defI);

  release(top);
}

void TestDUChain::testDeclareStruct()
{
  TEST_FILE_PARSE_ONLY

  //                 0         1         2         3         4         5         6         7
  //                 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  QByteArray method("struct A { int i; A(int b, int c) : i(c) { } virtual void test(int j) = 0; }; A instance;");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 1);
  QCOMPARE(top->localDeclarations().count(), 2);
  QVERIFY(top->localScopeIdentifier().isEmpty());

  IdentifiedType* idType = dynamic_cast<IdentifiedType*>(top->localDeclarations()[1]->abstractType().data());
  QVERIFY(idType);
  QCOMPARE( idType->identifier(), QualifiedIdentifier("A") );
  
  Declaration* defStructA = top->localDeclarations().first();
  QCOMPARE(defStructA->identifier(), Identifier("A"));
  QCOMPARE(defStructA->uses().count(), 0);
  QVERIFY(defStructA->type<CppClassType>());
  QCOMPARE(defStructA->type<CppClassType>()->elements().count(), 3);
  QCOMPARE(defStructA->type<CppClassType>()->elements().first(), typeInt);
  QVERIFY(CppFunctionType::Ptr::dynamicCast(defStructA->type<CppClassType>()->elements()[1]));
  QCOMPARE(defStructA->type<CppClassType>()->classType(), CppClassType::Struct);

  DUContext* structA = top->childContexts().first();
  QVERIFY(structA->parentContext());
  QCOMPARE(structA->importedParentContexts().count(), 0);
  QCOMPARE(structA->childContexts().count(), 3);
  QCOMPARE(structA->localDeclarations().count(), 3);
  QCOMPARE(structA->localScopeIdentifier(), QualifiedIdentifier("A"));

  Declaration* defI = structA->localDeclarations().first();
  QCOMPARE(defI->identifier(), Identifier("i"));
  QCOMPARE(defI->uses().count(), 1);

  QCOMPARE(findDeclaration(structA,  Identifier("i")), defI);
  QCOMPARE(findDeclaration(structA,  Identifier("b")), noDef);
  QCOMPARE(findDeclaration(structA,  Identifier("c")), noDef);

  DUContext* ctorImplCtx = structA->childContexts()[1];
  QVERIFY(ctorImplCtx->parentContext());
  QCOMPARE(ctorImplCtx->importedParentContexts().count(), 1);
  QCOMPARE(ctorImplCtx->childContexts().count(), 1);
  QCOMPARE(ctorImplCtx->localDeclarations().count(), 0);
  QVERIFY(ctorImplCtx->localScopeIdentifier().isEmpty());

  DUContext* ctorCtx = ctorImplCtx->importedParentContexts().first();
  QVERIFY(ctorCtx->parentContext());
  QCOMPARE(ctorCtx->childContexts().count(), 0);
  QCOMPARE(ctorCtx->localDeclarations().count(), 2);
  QCOMPARE(ctorCtx->localScopeIdentifier(), QualifiedIdentifier("A")); ///@todo check if it should really be this way

  Declaration* defB = ctorCtx->localDeclarations().first();
  QCOMPARE(defB->identifier(), Identifier("b"));
  QCOMPARE(defB->uses().count(), 0);

  Declaration* defC = ctorCtx->localDeclarations()[1];
  QCOMPARE(defC->identifier(), Identifier("c"));
  QCOMPARE(defC->uses().count(), 1);

  QCOMPARE(findDeclaration(ctorCtx,  Identifier("i")), defI);
  QCOMPARE(findDeclaration(ctorCtx,  Identifier("b")), defB);
  QCOMPARE(findDeclaration(ctorCtx,  Identifier("c")), defC);

  DUContext* testCtx = structA->childContexts().last();
  QCOMPARE(testCtx->childContexts().count(), 0);
  QCOMPARE(testCtx->localDeclarations().count(), 1);
  QCOMPARE(testCtx->localScopeIdentifier(), QualifiedIdentifier("test")); ///@todo check if it should really be this way

  Declaration* defJ = testCtx->localDeclarations().first();
  QCOMPARE(defJ->identifier(), Identifier("j"));
  QCOMPARE(defJ->uses().count(), 0);

  /*DUContext* insideCtorCtx = ctorCtx->childContexts().first();
  QCOMPARE(insideCtorCtx->childContexts().count(), 0);
  QCOMPARE(insideCtorCtx->localDeclarations().count(), 0);
  QVERIFY(insideCtorCtx->localScopeIdentifier().isEmpty());*/

  release(top);
}

void TestDUChain::testDeclareClass()
{
  TEST_FILE_PARSE_ONLY

  //                 0         1         2         3         4         5         6         7
  //                 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  QByteArray method("class A { void test(int); }; void A::test(int j) {}");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 3);
  QCOMPARE(top->localDeclarations().count(), 1);
  QVERIFY(top->localScopeIdentifier().isEmpty());

  Declaration* defClassA = top->localDeclarations().first();
  QCOMPARE(defClassA->identifier(), Identifier("A"));
  QCOMPARE(defClassA->uses().count(), 0);
  QVERIFY(defClassA->type<CppClassType>());
  QCOMPARE(defClassA->type<CppClassType>()->elements().count(), 1);
  CppFunctionType::Ptr function = CppFunctionType::Ptr::dynamicCast(defClassA->type<CppClassType>()->elements().first());
  QVERIFY(function);
  QCOMPARE(function->returnType(), typeVoid);
  QCOMPARE(function->arguments().count(), 1);
  QCOMPARE(function->arguments().first(), typeInt);

  DUContext* classA = top->childContexts().first();
  QVERIFY(classA->parentContext());
  QCOMPARE(classA->importedParentContexts().count(), 0);
  QCOMPARE(classA->childContexts().count(), 1);
  QCOMPARE(classA->localDeclarations().count(), 1);
  QCOMPARE(classA->localScopeIdentifier(), QualifiedIdentifier("A"));

  Declaration* defTest = classA->localDeclarations().first();
  Q_UNUSED(defTest);

  release(top);
}

void TestDUChain::testDeclareNamespace()
{
  TEST_FILE_PARSE_ONLY

  //                 0         1         2         3         4         5         6         7
  //                 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  QByteArray method("namespace foo { int bar; } int bar; int test() { return foo::bar; }");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 3);
  QCOMPARE(top->localDeclarations().count(), 2);
  QVERIFY(top->localScopeIdentifier().isEmpty());
  QCOMPARE(findDeclaration(top, Identifier("foo")), noDef);

  QCOMPARE(top->usingNamespaces().count(), 0);

  DUContext* fooCtx = top->childContexts().first();
  QCOMPARE(fooCtx->childContexts().count(), 0);
  QCOMPARE(fooCtx->localDeclarations().count(), 1);
  QCOMPARE(fooCtx->localScopeIdentifier(), QualifiedIdentifier("foo"));
  QCOMPARE(fooCtx->scopeIdentifier(), QualifiedIdentifier("foo"));

  DUContext* testCtx = top->childContexts()[2];
  QCOMPARE(testCtx->childContexts().count(), 0);
  QCOMPARE(testCtx->localDeclarations().count(), 0);
  QCOMPARE(testCtx->localScopeIdentifier(), QualifiedIdentifier());
  QCOMPARE(testCtx->scopeIdentifier(), QualifiedIdentifier());

  Declaration* bar2 = top->localDeclarations().first();
  QCOMPARE(bar2->identifier(), Identifier("bar"));
  QCOMPARE(bar2->qualifiedIdentifier(), QualifiedIdentifier("bar"));
  QCOMPARE(bar2->uses().count(), 0);

  Declaration* bar = fooCtx->localDeclarations().first();
  QCOMPARE(bar->identifier(), Identifier("bar"));
  QCOMPARE(bar->qualifiedIdentifier(), QualifiedIdentifier("foo::bar"));
  QCOMPARE(findDeclaration(testCtx,  QualifiedIdentifier("foo::bar")), bar);
  QCOMPARE(bar->uses().count(), 1);
  QCOMPARE(findDeclaration(top, bar->identifier()), bar2);
  QCOMPARE(findDeclaration(top, bar->qualifiedIdentifier()), bar);

  QCOMPARE(findDeclaration(top, QualifiedIdentifier("bar")), bar2);
  QCOMPARE(findDeclaration(top, QualifiedIdentifier("::bar")), bar2);
  QCOMPARE(findDeclaration(top, QualifiedIdentifier("foo::bar")), bar);
  QCOMPARE(findDeclaration(top, QualifiedIdentifier("::foo::bar")), bar);

  release(top);
}

void TestDUChain::testDeclareUsingNamespace()
{
  TEST_FILE_PARSE_ONLY

  //                 0         1         2         3         4         5         6         7
  //                 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  QByteArray method("namespace foo { int bar; } using namespace foo; int test() { return bar; }");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  QVERIFY(!top->parentContext());
  QCOMPARE(top->childContexts().count(), 3);
  QCOMPARE(top->localDeclarations().count(), 1);
  QVERIFY(top->localScopeIdentifier().isEmpty());
  QCOMPARE(findDeclaration(top, Identifier("foo")), noDef);

  QCOMPARE(top->usingNamespaces().count(), 1);
  QCOMPARE(top->usingNamespaces().first()->nsIdentifier, QualifiedIdentifier("foo"));
  QCOMPARE(top->usingNamespaces().first()->textCursor(), Cursor(0, 47));

  DUContext* fooCtx = top->childContexts().first();
  QCOMPARE(fooCtx->childContexts().count(), 0);
  QCOMPARE(fooCtx->localDeclarations().count(), 1);
  QCOMPARE(fooCtx->localScopeIdentifier(), QualifiedIdentifier("foo"));
  QCOMPARE(fooCtx->scopeIdentifier(), QualifiedIdentifier("foo"));

  Declaration* bar = fooCtx->localDeclarations().first();
  QCOMPARE(bar->identifier(), Identifier("bar"));
  QCOMPARE(bar->qualifiedIdentifier(), QualifiedIdentifier("foo::bar"));
  QCOMPARE(bar->uses().count(), 1);
  QCOMPARE(findDeclaration(top, bar->identifier(), top->textRange().start()), noDef);
  QCOMPARE(findDeclaration(top, bar->identifier()), bar);
  QCOMPARE(findDeclaration(top, bar->qualifiedIdentifier()), bar);

  QCOMPARE(findDeclaration(top, QualifiedIdentifier("bar")), bar);
  QCOMPARE(findDeclaration(top, QualifiedIdentifier("::bar")), noDef);
  QCOMPARE(findDeclaration(top, QualifiedIdentifier("foo::bar")), bar);
  QCOMPARE(findDeclaration(top, QualifiedIdentifier("::foo::bar")), bar);

  DUContext* testCtx = top->childContexts()[2];
  QVERIFY(testCtx->parentContext());
  QCOMPARE(testCtx->importedParentContexts().count(), 1);
  QCOMPARE(testCtx->childContexts().count(), 0);
  QCOMPARE(testCtx->localDeclarations().count(), 0);
  QCOMPARE(testCtx->localScopeIdentifier(), QualifiedIdentifier());
  QCOMPARE(testCtx->scopeIdentifier(), QualifiedIdentifier());

  release(top);
}

void TestDUChain::testTypedef() {
  QByteArray method("class A { }; typedef A B;");

  DUContext* top = parse(method, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  Declaration* defClassA = top->localDeclarations().first();
  QCOMPARE(defClassA->identifier(), Identifier("A"));
  QVERIFY(defClassA->type<CppClassType>());

  DUContext* classA = top->childContexts().first();
  QVERIFY(classA->parentContext());
  QCOMPARE(classA->importedParentContexts().count(), 0);
  QCOMPARE(classA->localScopeIdentifier(), QualifiedIdentifier("A"));

  QCOMPARE(findDeclaration(top,  Identifier("B"))->abstractType(), defClassA->abstractType());
  QVERIFY(findDeclaration(top,  Identifier("B"))->isTypeAlias());
  QCOMPARE(findDeclaration(top,  Identifier("B"))->kind(), Declaration::Type);
  
  QVERIFY( !dynamic_cast<CppTypeAliasType*>(findDeclaration(top,  Identifier("B"))->abstractType().data()) ); //Just verify that CppTypeAliasType is not used, because it isn't(maybe remove that class?)
  
  release(top);
}

void TestDUChain::testTemplates() {
  QByteArray method("template<class T> T test(const T& t) {}; template<class T, class T2> class A {T a; typedef T Template1; }; class B{int b;}; class C{int c;}; template<class T>class A<B,T>{};  typedef A<B,C> D;");

  QByteArray test2("template<class T> struct Scope1 { struct Scope2 { typedef T Test; } };");
  QByteArray test3("template<class T> class OtherScope {typedef T Test; }; template<class T> struct Scope1 { struct Scope2 { typedef OtherScope<T>::Test Test; } };");
  
  DUContext* top = parse(method, DumpAll);

  DUChainWriteLocker lock(DUChain::lock());

  Declaration* defTest = top->localDeclarations()[0];
  QCOMPARE(defTest->identifier(), Identifier("test"));
  QVERIFY(defTest->type<FunctionType>());
  QVERIFY( isTemplateDeclaration(defTest) );
  
  Declaration* defClassA = top->localDeclarations()[1];
  QCOMPARE(defClassA->identifier(), Identifier("A"));
  QVERIFY(defClassA->type<CppClassType>());
  QVERIFY( isTemplateDeclaration(defClassA) );

  Declaration* defClassB = top->localDeclarations()[2];
  QCOMPARE(defClassB->identifier(), Identifier("B"));
  QVERIFY(defClassB->type<CppClassType>());
  QVERIFY( !isTemplateDeclaration(defClassB) );
  
  Declaration* defClassC = top->localDeclarations()[3];
  QCOMPARE(defClassC->identifier(), Identifier("C"));
  QVERIFY(defClassC->type<CppClassType>());
  QVERIFY( !isTemplateDeclaration(defClassC) );
  
  DUContext* classA = defClassA->internalContext();
  QVERIFY(classA);
  QVERIFY(classA->parentContext());
  QCOMPARE(classA->importedParentContexts().count(), 1); //The template-parameter context is imported
  QCOMPARE(classA->localScopeIdentifier(), QualifiedIdentifier("A"));

  DUContext* classB = defClassB->internalContext();
  QVERIFY(classB);
  QVERIFY(classB->parentContext());
  QCOMPARE(classB->importedParentContexts().count(), 0);
  QCOMPARE(classB->localScopeIdentifier(), QualifiedIdentifier("B"));
  
  DUContext* classC = defClassC->internalContext();
  QVERIFY(classC);
  QVERIFY(classC->parentContext());
  QCOMPARE(classC->importedParentContexts().count(), 0);
  QCOMPARE(classC->localScopeIdentifier(), QualifiedIdentifier("C"));

  ///Test getting the typedef for the unset template
  Declaration* typedefDecl = findDeclaration(classA, Identifier("Template1"));
  QVERIFY(typedefDecl);
  QVERIFY(typedefDecl->isTypeAlias());
  QVERIFY(typedefDecl->abstractType());
  QVERIFY(getDeclaration(typedefDecl->abstractType()));
  QVERIFY(dynamic_cast<CppTemplateParameterType*>(typedefDecl->abstractType().data()));
  Declaration* templateDecl = getDeclaration( typedefDecl->abstractType() );
  QVERIFY(templateDecl);
  QCOMPARE(templateDecl->qualifiedIdentifier(), QualifiedIdentifier("T"));

  ///Test creating a template instance of class A<B,C>
  Identifier ident("A");
  ident.appendTemplateIdentifier(QualifiedIdentifier("B"));
  ident.appendTemplateIdentifier(QualifiedIdentifier("C"));
  Declaration* instanceDefClassA = findDeclaration(top, ident);
  QVERIFY(instanceDefClassA);
/*  QCOMPARE(findDeclaration(top,  Identifier("B"))->abstractType(), defClassA->abstractType());
  QVERIFY(findDeclaration(top,  Identifier("B"))->isTypeAlias());
  QCOMPARE(findDeclaration(top,  Identifier("B"))->kind(), Declaration::Type);*/
  release(top);
}

void TestDUChain::testFileParse()
{
  //QSKIP("Unwanted", SkipSingle);

  //QFile file("/opt/kde4/src/kdevelop/languages/cpp/duchain/tests/files/membervariable.cpp");
  QFile file("/opt/kde4/src/kdevelop/languages/csharp/parser/csharp_parser.cpp");
  //QFile file("/opt/kde4/src/kdevelop/plugins/outputviews/makewidget.cpp");
  //QFile file("/opt/kde4/src/kdelibs/kate/part/katecompletionmodel.h");
  //QFile file("/opt/kde4/src/kdevelop/lib/kdevbackgroundparser.cpp");
  //QFile file("/opt/qt-copy/src/gui/kernel/qwidget.cpp");
  QVERIFY( file.open( QIODevice::ReadOnly ) );

  QByteArray fileData = file.readAll();
  file.close();
  QString contents = QString::fromUtf8( fileData.constData() );
  rpp::Preprocessor preprocessor;
  QString ppd = preprocessor.processString( contents );
  QByteArray preprocessed = ppd.toUtf8();

  DUContext* top = parse(preprocessed, DumpNone);

  DUChainWriteLocker lock(DUChain::lock());

  SymbolTable::self()->dumpStatistics();

  release(top);
}

void TestDUChain::testHashedStringRepository() {
  /*
New Problem:
Have sets:
a,b
c,d
a,c
b,d
create set:
-> a,b,c,d

a,b,c,d should be a unique set, but it can be constructed from a,b + c,d and from a,c + b,d
Solution:
Merge a,b+a,c, a,c+b,d and then merge those to a,b,c,d
*/
  {
    HashedStringRepository* rep = new HashedStringRepository();
    QList<HashedStringSubset*> set1Strings;
    set1Strings << rep->getAtomicSubset(HashedString("a"));
    set1Strings << rep->getAtomicSubset(HashedString("b"));
    HashedStringSubset* set1 = rep->buildSet(set1Strings);
    
    QList<HashedStringSubset*> set2Strings;
    set2Strings << rep->getAtomicSubset(HashedString("c"));
    set2Strings << rep->getAtomicSubset(HashedString("d"));
    HashedStringSubset* set2 = rep->buildSet(set2Strings);
    
    QList<HashedStringSubset*> set3Strings;
    set3Strings << rep->getAtomicSubset(HashedString("a"));
    set3Strings << rep->getAtomicSubset(HashedString("c"));
    HashedStringSubset* set3 = rep->buildSet(set3Strings);

    QList<HashedStringSubset*> set4Strings;
    set4Strings << rep->getAtomicSubset(HashedString("b"));
    set4Strings << rep->getAtomicSubset(HashedString("d"));
    HashedStringSubset* set4 = rep->buildSet(set4Strings);
    
    QList<HashedStringSubset*> set5Strings;
    set5Strings << rep->getAtomicSubset(HashedString("a"));
    set5Strings << rep->getAtomicSubset(HashedString("b"));
    set5Strings << rep->getAtomicSubset(HashedString("c"));
    set5Strings << rep->getAtomicSubset(HashedString("d"));
    HashedStringSubset* set5 = rep->buildSet(set5Strings);

    Q_ASSERT(rep->intersection(set1, set5) == set1);
    Q_ASSERT(rep->intersection(set2, set5) == set2);
    Q_ASSERT(rep->intersection(set3, set5) == set3);
    Q_ASSERT(rep->intersection(set4, set5) == set4);
    
    kDebug() << "dot-graph: \n" << rep->dumpDotGraph() << "\n";
    
    delete rep;
  }
  
  Q_ASSERT(0);

/*
@todo first compute the hash correctly, then solve the problem above
@todo implement intersection
 
Intersection assumption:
If there is set A and set B, there is also a set that represents the intersection of A and B

test:
atomics are: a,b,c,d,e
Buld set a,b,c and b,c,d:
a,b,c:
a,c
a,b
b,c,d:
b,d
c,d

- no intersection-set b,c

  */
    HashedStringRepository* rep = new HashedStringRepository();
  
  QList<HashedStringSubset*> set1Strings;
  set1Strings << rep->getAtomicSubset(HashedString("a"));
  set1Strings << rep->getAtomicSubset(HashedString("b"));
  set1Strings << rep->getAtomicSubset(HashedString("c"));
  set1Strings << rep->getAtomicSubset(HashedString("d"));
  HashedStringSubset* set1 = rep->buildSet(set1Strings);
  
  QList<HashedStringSubset*> set2Strings;
  set2Strings << rep->getAtomicSubset(HashedString("a"));
  set2Strings << rep->getAtomicSubset(HashedString("b"));
  set2Strings << rep->getAtomicSubset(HashedString("c"));
  HashedStringSubset* set2 = rep->buildSet(set2Strings);
  
  QList<HashedStringSubset*> set3Strings;
  set3Strings << rep->getAtomicSubset(HashedString("a"));
  set3Strings << rep->getAtomicSubset(HashedString("b"));
  set3Strings << rep->getAtomicSubset(HashedString("c"));
  set3Strings << rep->getAtomicSubset(HashedString("e"));
  HashedStringSubset* set3 = rep->buildSet(set3Strings);

  QList<HashedStringSubset*> set4Strings;
  set4Strings << rep->getAtomicSubset(HashedString("e"));
  set4Strings << rep->getAtomicSubset(HashedString("a"));
  HashedStringSubset* set4 = rep->buildSet(set4Strings);

  kDebug() << "dot-graph: \n" << rep->dumpDotGraph() << "\n";
  
  delete rep;
  
}

void TestDUChain::release(DUContext* top)
{
  KDevelop::EditorIntegrator::releaseTopRange(top->textRangePtr());
  delete top;
}

DUContext* TestDUChain::parse(const QByteArray& unit, DumpAreas dump)
{
  if (dump)
    kDebug() << "==== Beginning new test case...:" << endl << unit << endl << endl;

  ParseSession* session = new ParseSession();
  session->setContents(unit);

  Parser parser(&control);
  TranslationUnitAST* ast = parser.parse(session);
  ast->session = session;

  if (dump & DumpAST) {
    kDebug() << "===== AST:" << endl;
    dumper.dump(ast, session);
  }

  static int testNumber = 0;
  KUrl url(QString("file:///internal/%1").arg(testNumber++));

  DeclarationBuilder definitionBuilder(session);
  Cpp::LexedFilePointer file( new Cpp::LexedFile( url, 0 ) );
  TopDUContext* top = definitionBuilder.buildDeclarations(file, ast);

  UseBuilder useBuilder(session);
  useBuilder.buildUses(ast);

  if (dump & DumpDUChain) {
    kDebug() << "===== DUChain:" << endl;

    DUChainWriteLocker lock(DUChain::lock());
    dumper.dump(top);
  }

  if (dump & DumpType) {
    kDebug() << "===== Types:" << endl;
    DUChainWriteLocker lock(DUChain::lock());
    DumpTypes dt;
    foreach (const AbstractType::Ptr& type, definitionBuilder.topTypes())
      dt.dump(type.data());
  }

  if (dump)
    kDebug() << "===== Finished test case." << endl;

  delete session;

  return top;
}

#include "test_duchain.moc"
