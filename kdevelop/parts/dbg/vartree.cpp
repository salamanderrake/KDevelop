// **************************************************************************
//                          vartree.cpp  -  description
//                             -------------------
//    begin                : Sun Aug 8 1999
//    copyright            : (C) 1999 by John Birch
//    email                : jb.nz@writeme.com
// **************************************************************************

// **************************************************************************
// *                                                                        *
// *   This program is free software; you can redistribute it and/or modify *
// *   it under the terms of the GNU General Public License as published by *
// *   the Free Software Foundation; either version 2 of the License, or    *
// *   (at your option) any later version.                                  *
// *                                                                        *
// **************************************************************************

#include "vartree.h"
#include "gdbparser.h"

#include <klocale.h>
#include <kpopupmenu.h>

#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qregexp.h>

#if defined(DBG_MONITOR)
  #define DBG_DISPLAY(X)          {emit rawData(QString(X));}
#else
  #define DBG_DISPLAY(X)          {;}
#endif

// **************************************************************************

//TODO - change to a base class parser and setup a factory
static GDBParser* parser = 0;

static GDBParser* getParser()
{
  if (!parser)
    parser = new GDBParser;

  return parser;
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

VarViewer::VarViewer( QWidget *parent, const char *name ) :
  QWidget( parent, name )
{
  setFocusPolicy(QWidget::StrongFocus);
  QVBoxLayout *topLayout = new QVBoxLayout(this, 2);

  varTree_ = new VarTree(this);
  topLayout->addWidget( varTree_, 10 );

  QBoxLayout *watchEntry = new QHBoxLayout();
  topLayout->addLayout( watchEntry );

  QLabel *label = new QLabel( i18n("Watch: "), this );
  label->setMinimumSize( label->sizeHint() );
  label->setMaximumSize( label->sizeHint() );
  watchEntry->addWidget( label );

  // make the size small so that it can fit within the parent widget
  // size. The parents size is currently 4 tabs wide with <=3chars
  // in each tab. (ie quite small!) 
  watchVarEntry_ = new QLineEdit(this);
  watchVarEntry_->setMinimumSize(0,0); //watchVarEntry_->sizeHint() );
  watchEntry->addWidget( watchVarEntry_ );

  // just add a bit of space at the end of the entry widget
//  QLabel *blank = new QLabel( " ", this );
//  blank->setMinimumSize( blank->sizeHint() );
//  blank->setMaximumSize( blank->sizeHint() );
//  watchEntry->addWidget( blank );

  QPushButton* addButton = new QPushButton( i18n("Add"), this );
  addButton->setMinimumSize( addButton->sizeHint() );
  addButton->setMaximumSize( addButton->sizeHint() );
  watchEntry->addWidget( addButton );

  connect(addButton, SIGNAL(clicked()), SLOT(slotAddWatchVariable()));
  connect(watchVarEntry_, SIGNAL(returnPressed()), SLOT(slotAddWatchVariable()));

  topLayout->activate();
}

// **************************************************************************

void VarViewer::clear()
{
  varTree_->clear();
}

// **************************************************************************

void VarViewer::slotAddWatchVariable()
{
  QString watchVar(watchVarEntry_->text());
  if (!watchVar.isEmpty())
    varTree_->slotAddWatchVariable(watchVar);
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

VarTree::VarTree( QWidget *parent, const char *name ) :
  QListView(parent, name),
  activeFlag_(0)
{
  setRootIsDecorated(true);
  setSorting(-1);
  setFrameStyle(Panel | Sunken);
  setLineWidth(2);
  addColumn(i18n("Variable"));
  addColumn(i18n("Value"));
  setMultiSelection (false);
  setFocusPolicy(QWidget::StrongFocus);

  connect (this,  SIGNAL(rightButtonClicked ( QListViewItem *, const QPoint &, int )),
                  SLOT(slotRightButtonClicked ( QListViewItem *, const QPoint &, int )));
}

// **************************************************************************

VarTree::~VarTree()
{
}

// **************************************************************************

void VarTree::slotRightButtonClicked( QListViewItem* selectedItem,
                                      const QPoint &,
                                      int)
{
  if (!selectedItem)
    return;

  setSelected (selectedItem, true);    // Need to select this item.
  if (selectedItem->parent())
  {
    KPopupMenu popup(selectedItem->text(VarNameCol));
    TrimmableItem* item = static_cast<TrimmableItem*>(findRoot(selectedItem));
    if (item && item->isA("WatchRoot"))
      popup.insertItem( i18n("Delete watch variable"), this, SLOT(slotRemoveWatchVariable()) );
    popup.insertItem( i18n("Toggle watchpoint"), this, SLOT(slotToggleWatchpoint()) );
    popup.exec(QCursor::pos());
  }
}

// **************************************************************************

void VarTree::slotToggleWatchpoint()
{
  TrimmableItem* item = static_cast<TrimmableItem*>(currentItem());
  if (item && item->isA("VarItem"))
    emit toggleWatchpoint((static_cast<VarItem*>(item))->fullName());
}

// **************************************************************************

void VarTree::slotRemoveWatchVariable()
{
  delete currentItem();
}

// **************************************************************************

void VarTree::slotAddWatchVariable(const QString& watchVar)
{
  VarItem* varItem = new VarItem(findWatch(), watchVar, typeUnknown);
  emitExpandItem(varItem);
}

// **************************************************************************

void VarTree::emitSetLocalViewState(bool localsOn, int frameNo)
{
  // When they want to _close_ a frame then we need to check the state of
  // all other frames to determine whether we still need the locals.
  if (!localsOn)
  {
    TrimmableItem* sibling = static_cast<TrimmableItem*>(firstChild());
    while (sibling)
    {
      if (sibling->isA("FrameRoot"))
      {

        FrameRoot* frame = static_cast<FrameRoot*> (sibling);
        if (frame->isOpen())
        {
          localsOn = true;
          break;
        }
      }

      sibling = static_cast<TrimmableItem*>(sibling->nextSibling());
    }
  }

  emit setLocalViewState(localsOn);
  emit selectFrame(frameNo);
}

// **************************************************************************

QListViewItem* VarTree::findRoot(QListViewItem* item) const
{
  while (item->parent())
    item = item->parent();

  return item;
}

// **************************************************************************

FrameRoot* VarTree::findFrame(int frameNo) const
{
  TrimmableItem* sibling = static_cast<TrimmableItem*> (firstChild());

  // frames only exist on th top level so we only need to
  // check the siblings
  while (sibling)
  {
    if (sibling->isA("FrameRoot"))
    {
      FrameRoot* frame = static_cast<FrameRoot*> (sibling);
      if (frame->getFrameNo() == frameNo)
        return frame;
    }

    sibling = static_cast<TrimmableItem*> (sibling->nextSibling());
  }

  return 0;
}

// **************************************************************************

WatchRoot* VarTree::findWatch()
{
  TrimmableItem* sibling = static_cast<TrimmableItem*>(firstChild());

  while (sibling)
  {
    if (sibling->isA("WatchRoot"))
      return static_cast<WatchRoot*>(sibling);

    sibling = static_cast<TrimmableItem*>(sibling->nextSibling());
  }

  return new WatchRoot(this);
}

// **************************************************************************

void VarTree::trim()
{
  TrimmableItem* child = static_cast<TrimmableItem*>(firstChild());
  while (child)
  {
    TrimmableItem* nextChild = static_cast<TrimmableItem*>(child->nextSibling());

    // don't trim the watch root
    if (!child->isA("WatchRoot"))
    {
      if (child->isActive())
        child->trim();
      else
        delete child;
    }
    child = nextChild;
  }
}

// **************************************************************************

void VarTree::trimExcessFrames()
{
  TrimmableItem* child = static_cast<TrimmableItem*> (firstChild());
  while (child)
  {
    TrimmableItem* nextChild = static_cast<TrimmableItem*> (child->nextSibling());
    if (child->isA("FrameRoot"))
    {
      FrameRoot* frame = static_cast<FrameRoot*> (child);
      if (frame->getFrameNo() != 0)
        delete frame;
    }

    child = nextChild;
  }
}

// **************************************************************************

QListViewItem* VarTree::lastChild() const
{
  QListViewItem* child = firstChild();
  if (child)
    while (QListViewItem* nextChild = child->nextSibling())
      child = nextChild;

  return child;
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

TrimmableItem::TrimmableItem(VarTree* parent) :
  QListViewItem (parent, parent->lastChild()),
  activeFlag_(0)
{
  setActive();
}

// **************************************************************************

TrimmableItem::TrimmableItem(TrimmableItem* parent) :
  QListViewItem (parent, parent->lastChild()),
  activeFlag_(0)
{
  setActive();
}

// **************************************************************************

TrimmableItem::~TrimmableItem()
{
}

// **************************************************************************

int TrimmableItem::rootActiveFlag() const
{
  return ((VarTree*)listView())->activeFlag();
}

// **************************************************************************

QListViewItem* TrimmableItem::lastChild() const
{
  QListViewItem* child = firstChild();
  if (child)
    while (QListViewItem* nextChild = child->nextSibling())
      child = nextChild;

  return child;
}

// **************************************************************************

TrimmableItem* TrimmableItem::findMatch
                (const QString& match, DataType type) const
{
  TrimmableItem* child = static_cast<TrimmableItem*> (firstChild());

 // Check the siblings on this branch
  while (child)
  {
    if ( (child->text(VarNameCol) == match) && (child->getDataType() == type))
      return child;

    child = static_cast<TrimmableItem*> (child->nextSibling());
  }

  return 0;
}

// **************************************************************************

void TrimmableItem::trim()
{
  TrimmableItem* child = static_cast<TrimmableItem*> (firstChild());
  while (child)
  {
    TrimmableItem* nextChild = static_cast<TrimmableItem*> (child->nextSibling());
    // Never trim a branch if we are waiting on data to arrive.
    if (!isOpen() || getDataType() != typePointer)
    {
      if (child->isActive())
        child->trim();      // recurse
      else
        delete child;
    }
    child = nextChild;
  }
}

// **************************************************************************

DataType TrimmableItem::getDataType() const
{
  return typeUnknown;
}

// **************************************************************************

void TrimmableItem::setCache(const QString&)
{
  ASSERT(false);
}

// **************************************************************************

QString TrimmableItem::getCache()
{
  ASSERT(false);
  return QString();
}

// **************************************************************************

void TrimmableItem::updateValue(const QString&)
{
}

// **************************************************************************

QString TrimmableItem::key (int, bool) const
{
  return "";
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

VarItem::VarItem( TrimmableItem* parent, const QString& varName, DataType dataType) :
  TrimmableItem (parent),
  cache_(QString()),
  dataType_(dataType),
  highlight_(false)
{
  setText (VarNameCol, varName);
}

// **************************************************************************

VarItem::~VarItem()
{
}

// **************************************************************************

QString VarItem::varPath() const
{
  QString varPath("");
  const TrimmableItem* item = this;

  // This stops at the root item (FrameRoot or WatchRoot)
  while ((item = static_cast<const TrimmableItem*> (item->parent())))
  {
    if (!item->isA("VarItem"))
      break;

    if (item->getDataType() != typeArray)
    {
      if (*(item->text(VarNameCol)) != '<')
      {
        QString itemName(item->text(VarNameCol));
        varPath = itemName.replace(QRegExp("^static "), "") + "." + varPath;
      }
    }
  }
  return varPath;
}

// **************************************************************************

QString VarItem::fullName() const
{
  QString itemName(getName());
  ASSERT (itemName);
  if (itemName[0] == '<')
    return varPath();

  return varPath() + itemName.replace(QRegExp("^static "), "");
}

// **************************************************************************

void VarItem::setText ( int column, const char * data )
{
  if (!isActive() && isOpen() && dataType_ == typePointer)
    ((VarTree*)listView())->emitExpandItem(this);

  setActive();
  if (column == ValueCol)
  {
    QString oldValue(text(column));
    if (oldValue)                   // Don't highlight new items
      highlight_ = (oldValue != QString(data));
  }

  QListViewItem::setText(column, data);
}

// **************************************************************************

void VarItem::updateValue(char* data)
{
  getParser()->parseData(this, data, true, false);
  setActive();
}

// **************************************************************************

void VarItem::setCache(const QString& value)
{
  cache_ = value;
  setExpandable(true);
  checkForRequests();
  if (isOpen())
    setOpen(true);
  setActive();
}

// **************************************************************************

void VarItem::setOpen(bool open)
{
  if (open)
  {
    if (cache_)
    {
      QString value = cache_;
      cache_ = QString();
      getParser()->parseData(this, (char*) value.data(), false, false);
      trim();
    }
    else
      if (dataType_ == typePointer || dataType_ == typeReference)
        ((VarTree*)listView())->emitExpandItem(this);
  }

  QListViewItem::setOpen(open);
}

// **************************************************************************

QString VarItem::getCache()
{
  return cache_;
}

// **************************************************************************

void VarItem::checkForRequests()
{
  // TODO - hardcoded for now - these should get read from config

  // Signature for a QT1.44 QString
  if (strncmp(cache_, "<QArrayT<char>> = {<QGArray> = {shd = ", 38) == 0)
    ((VarTree*)listView())->emitExpandUserItem(this,
                                          fullName()+QString(".shd.data"));

  // Signature for a QT1.44 QDir
  if (strncmp(cache_, "dPath = {<QArrayT<char>> = {<QGArray> = {shd", 44) == 0)
    ((VarTree*)listView())->emitExpandUserItem(this,
                                          fullName()+QString(".dPath.shd.data"));

  // Signature for a QT2.0.x QT2.1 QString
  // TODO - This handling is not that good - but it works sufficiently well
  // at the moment to leave it here, and it won't cause bad things to happen.
  if (strncmp(cache_, "d = 0x", 6) == 0)      // Eeeek - too small
    ((VarTree*)listView())->emitExpandUserItem(this,
           QString().sprintf("(($len=($data=%s.d).len)?$data.unicode.rw@($len>100?200:$len*2):\"\")",
           fullName().data()));
}

// **************************************************************************

DataType VarItem::getDataType() const
{
  return dataType_;
}

// **************************************************************************

// Overridden to highlight the changed items
void VarItem::paintCell( QPainter * p, const QColorGroup & cg,
                                int column, int width, int align )
{
  if ( !p )
    return;

  if (column == ValueCol && highlight_)
  {
    QColorGroup hl_cg( cg.foreground(), cg.background(), cg.light(),
                        cg.dark(), cg.mid(), red, cg.base());
    QListViewItem::paintCell( p, hl_cg, column, width, align );
  }
  else
    QListViewItem::paintCell( p, cg, column, width, align );
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

FrameRoot::FrameRoot(VarTree* parent, int frameNo) :
  TrimmableItem (parent),
  needLocals_(true),
  frameNo_(frameNo),
  params_(QString()),
  locals_(QString())
{
  setExpandable(true);
}

// **************************************************************************

FrameRoot::~FrameRoot()
{
}

// **************************************************************************

void FrameRoot::setLocals(char* locals)
{

  ASSERT(isActive());

  // "No symbol table info available" or "No locals."
  bool noLocals = (locals &&  (strncmp(locals, "No ", 3) == 0));
  setExpandable(!params_.isEmpty() || !noLocals);

  if (noLocals)
  {
    locals_ = "";
    if (locals)
      if (char* end = strchr(locals, '\n'))
        *end = 0;      // clobber the new line
  }
  else
    locals_ = locals;

  if (!isExpandable() && noLocals)
    setText ( ValueCol, locals );

  needLocals_ = false;
  if (isOpen())
    setOpen(true);
}

// **************************************************************************

void FrameRoot::setParams(const QString& params)
{
  setActive();
  params_ = params;
  needLocals_ = true;
}

// **************************************************************************

// Override setOpen so that we can decide what to do when we do change
// state. This
void FrameRoot::setOpen(bool open)
{
  bool localStateChange = (isOpen() != open);
  QListViewItem::setOpen(open);

  if (localStateChange)
    emit ((VarTree*)listView())->emitSetLocalViewState(open, frameNo_);

  if (!open)
    return;

  getParser()->parseData(this, (char*)params_.data(), false, true);
  getParser()->parseData(this, (char*)locals_.data(), false, false);

  locals_ = QString();
  params_ = QString();
}

// **************************************************************************
// **************************************************************************
// **************************************************************************
// **************************************************************************

WatchRoot::WatchRoot(VarTree* parent) :
  TrimmableItem(parent)
{
  setText(0, i18n("Watch"));
  setOpen(true);
}

// **************************************************************************

WatchRoot::~WatchRoot()
{
}

// **************************************************************************

void WatchRoot::requestWatchVars()
{
  for (QListViewItem* child = firstChild(); child; child = child->nextSibling())
  {
    TrimmableItem* item = static_cast<TrimmableItem*>(child);
    if (item->isA("VarItem"))
      ((VarTree*)listView())->emitExpandItem(static_cast<VarItem*>(item));
  }
}

// **************************************************************************
// **************************************************************************
// **************************************************************************
#include "vartree.moc"
