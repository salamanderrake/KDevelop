/***************************************************************************
 *   Copyright (C) 2001 by Jakob Simon-Gaarde                              *
 *   jsgaarde@tdcspace.dk                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _FILEBUFFER_H_
#define _FILEBUFFER_H_

#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include "caret.h"

class FileBuffer;
typedef QValueList<FileBuffer*> FileBufferList;

class FileBuffer
{
public:
  // Constructor/destructor
                  FileBuffer() {}
                  FileBuffer(const QString &fileName) {bufferFile(fileName);}
                  ~FileBuffer();

  // basic methods
  void            bufferFile(const QString &fileName);
  void            appendBufferText(const QStringList &buffer) {m_buffer+=buffer;}
  void            removeComments();
  Caret           findInBuffer(const QString &subString,const Caret& startPos,bool nvlToMax=false);
  void            saveBuffer(const QString &filename);
  void            dumpBuffer();
  QString         pop(int row);
  QStringList     popBlock(const Caret &blockStart, const Caret &blockEnd);
  QStringList     copyBlock(const Caret &blockStart, const Caret &blockEnd);

  // Scopes
  void            setScopeName(const QString &scopeName) {m_scopeName=scopeName;}
  QString         getScopeName() {return m_scopeName;}
  QStringList     getChildScopeNames();
  bool            findNextScope(const Caret &pos, Caret& scopeStart, Caret& scopeEnd);
  Caret           findScopeEnd(Caret pos);

  // Recursive scope methods
  bool            handleScopes();
  int             findChildBuffer(const QString &scopeName);
  void            makeScope(const QString &scopeString);
  QStringList     getBufferTextInDepth();
  FileBuffer*     getSubBuffer(QString scopeString="");
  void            splitScopeString(QString scopeString,QString &scopeName, QString &scopeRest);
  QStringList     getAllScopeStrings(int depth=0);
  QStringList     getAllScopeNames(int depth=0);

  // Variable value handling
  void            removeValues(const QString &variable);
  bool            getValues(const QString &variable,QStringList &plusValues, QStringList &minusValues);
  void            setValues(const QString &variable,QString values,int valuesPerRow=3,bool append=false)
                  {setValues(variable,QStringList::split(' ',values),valuesPerRow,append);}
  void            setValues(const QString &variable,QStringList values,int valuesPerRow=3,bool append=false);
  bool            getAllExcludeValues(const QString &variable,QStringList &minusValues,int depth=0);

private:
  QString         m_scopeName;
  QStringList     m_buffer;
  FileBufferList  m_subBuffers;
};

#endif

