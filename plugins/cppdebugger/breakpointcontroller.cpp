/* This file is part of the KDE project
   Copyright (C) 2002 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2006, 2008 Vladimir Prus <ghost@cs.msu.su>
   Copyright (C) 2007 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "breakpointcontroller.h"

#include "newbreakpoint.h"
#include "breakpoints.h"
#include <QPixmap>
#include <KIcon>
#include <KParts/PartManager>

#include <kdebug.h>
#include <klocale.h>
#include <ktexteditor/document.h>

#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/idocument.h>

#include "gdbcontroller.h"
#include "gdbcommand.h"
#include "breakpoint.h"

#include "util/treeitem.h"
#include "util/treemodel.h"

// #include "modeltest.h"

using namespace KTextEditor;
using namespace GDBDebugger;
using namespace GDBMI;

static int m_activeFlag = 0;

BreakpointController::BreakpointController(GDBController* parent)
: TreeModel(QVector<QString>() << "" << "" << "Type" << "Location" << "Condition",
            parent)
{
    universe_ = new Breakpoints(this, parent);
    setRootItem(universe_);
    universe_->load();
    universe_->createHelperBreakpoint();

    connect(this, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            universe_, SLOT(save()));
    connect(this, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            universe_, SLOT(save()));
    connect(this, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            universe_, SLOT(save()));

    //new ModelTest(this, this);

    foreach(KParts::Part* p, KDevelop::ICore::self()->partController()->parts())
        slotPartAdded(p);
    connect(KDevelop::ICore::self()->partController(),
            SIGNAL(partAdded(KParts::Part*)),
            this,
            SLOT(slotPartAdded(KParts::Part*)));

    // FIXME: maybe, all debugger components should derive from
    // a base class that does this connect.
    connect(parent,     SIGNAL(event(event_t)),
            this,       SLOT(slotEvent(event_t)));
}

void BreakpointController::slotPartAdded(KParts::Part* part)
{
    if (KTextEditor::Document* doc = dynamic_cast<KTextEditor::Document*>(part))
    {
        MarkInterface *iface = dynamic_cast<MarkInterface*>(doc);
        if( !iface )
            return;
        
        iface->setMarkDescription((MarkInterface::MarkTypes)BreakpointMark, i18n("Breakpoint"));
        iface->setMarkPixmap((MarkInterface::MarkTypes)BreakpointMark, *inactiveBreakpointPixmap());
        iface->setMarkPixmap((MarkInterface::MarkTypes)ActiveBreakpointMark, *activeBreakpointPixmap());
        iface->setMarkPixmap((MarkInterface::MarkTypes)ReachedBreakpointMark, *reachedBreakpointPixmap());
        iface->setMarkPixmap((MarkInterface::MarkTypes)DisabledBreakpointMark, *disabledBreakpointPixmap());
        iface->setMarkPixmap((MarkInterface::MarkTypes)ExecutionPointMark, *executionPointPixmap());
        iface->setEditableMarks( BookmarkMark | BreakpointMark );
        
        // When a file is loaded then we need to tell the editor (display window)
        // which lines contain a breakpoint.
        foreach (Breakpoint* breakpoint, breakpoints())
            adjustMark(breakpoint, true);
        
#if 0
        connect( doc, 
                 SIGNAL(markChanged(KTextEditor::Document*, KTextEditor::Mark, KTextEditor::MarkInterface::MarkChangeAction)), this, SLOT(markChanged(KTextEditor::Document*, KTextEditor::Mark, KTextEditor::MarkInterface::MarkChangeAction)) );
#endif
    }
}

BreakpointController::~BreakpointController()
{
}

Breakpoints* BreakpointController::breakpointsItem()
{
    return universe_;
}

QVariant 
BreakpointController::headerData(int section, Qt::Orientation orientation,
                                 int role) const
{ 
    if (orientation == Qt::Horizontal && role == Qt::DecorationRole
        && section == 0)
        return KIcon("dialog-ok-apply");
    else if (orientation == Qt::Horizontal && role == Qt::DecorationRole
             && section == 1)
        return KIcon("system-switch-user");

    return TreeModel::headerData(section, orientation, role);
}

Qt::ItemFlags BreakpointController::flags(const QModelIndex &index) const
{
    /* FIXME: all this logic must be in item */
    if (!index.isValid())
        return 0;

    if (index.column() == 0)
        return static_cast<Qt::ItemFlags>(
            Qt::ItemIsEnabled | Qt::ItemIsSelectable 
            | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);

    if (index.column() == NewBreakpoint::location_column 
        || index.column() == NewBreakpoint::condition_column)
        return static_cast<Qt::ItemFlags>(
            Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

    return static_cast<Qt::ItemFlags>(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

void BreakpointController::clearExecutionPoint()
{
    kDebug(9012) << "clearExecutionPoint";
    foreach (KDevelop::IDocument* document, KDevelop::ICore::self()->documentController()->openDocuments())
    {
        MarkInterface *iface = dynamic_cast<MarkInterface*>(document->textDocument());
        if (!iface)
            continue;

        QHashIterator<int, KTextEditor::Mark*> it = iface->marks();
        while (it.hasNext())
        {
            Mark* mark = it.next().value();
            if( mark->type & ExecutionPointMark )
                iface->removeMark( mark->line, ExecutionPointMark );
        }
    }
}


void BreakpointController::gotoExecutionPoint(const KUrl &url, int lineNum)
{
    clearExecutionPoint();
    kDebug(9012) << "gotoExecutionPoint";
    KDevelop::IDocument* document = KDevelop::ICore::self()
        ->documentController()
        ->openDocument(url, KTextEditor::Cursor(lineNum, 0));

    if( !document )
        return;

    MarkInterface *iface = dynamic_cast<MarkInterface*>(document->textDocument());
    if( !iface )
        return;

    document->textDocument()->blockSignals(true);
    iface->addMark( lineNum, ExecutionPointMark );
    document->textDocument()->blockSignals(false);
}

void BreakpointController::markChanged(
    KTextEditor::Document *document, 
    KTextEditor::Mark mark, 
    KTextEditor::MarkInterface::MarkChangeAction action)
{
#if 0
    int type = mark.type;
    /* Is this a breakpoint mark, to begin with? */
    if (type != (MarkInterface::MarkTypes)BreakpointMark
        && type != (MarkInterface::MarkTypes)ActiveBreakpointMark
        && type != (MarkInterface::MarkTypes)ReachedBreakpointMark
        && type != (MarkInterface::MarkTypes)DisabledBreakpointMark)
        return;

    switch (action) {
        case KTextEditor::MarkInterface::MarkAdded: {
            FilePosBreakpoint* fileBreakpoint = new FilePosBreakpoint(this, document->url().path(), mark.line);
            addBreakpoint(fileBreakpoint);
        }
            break;

        case KTextEditor::MarkInterface::MarkRemoved:
            // Find this breakpoint and delete it
            foreach (Breakpoint* breakpoint, m_breakpoints)
                if (FilePosBreakpoint* fileBreakpoint = qobject_cast<FilePosBreakpoint*>(breakpoint))
                    if (mark.line == fileBreakpoint->lineNum())
                        if (document->url().path() == fileBreakpoint->fileName()) {
                            fileBreakpoint->remove();
                            removeBreakpoint(fileBreakpoint);
                        }
            break;
    }

    if ( KDevelop::ICore::self()->documentController()->activeDocument() && KDevelop::ICore::self()->documentController()->activeDocument()->textDocument() == document )
    {
        //bring focus back to the editor
        // TODO probably want a different command here
        KDevelop::ICore::self()->documentController()->activateDocument(KDevelop::ICore::self()->documentController()->activeDocument());
    }
#endif
}

const QPixmap* BreakpointController::inactiveBreakpointPixmap()
{
  static QPixmap pixmap=KIcon("script-error").pixmap(QSize(22,22), QIcon::Normal, QIcon::Off);
  return &pixmap;
}

const QPixmap* BreakpointController::activeBreakpointPixmap()
{
  static QPixmap pixmap=KIcon("script-error").pixmap(QSize(22,22), QIcon::Active, QIcon::Off);
  return &pixmap;
}

const QPixmap* BreakpointController::reachedBreakpointPixmap()
{
  static QPixmap pixmap=KIcon("script-error").pixmap(QSize(22,22), QIcon::Selected, QIcon::Off);
  return &pixmap;
}

const QPixmap* BreakpointController::disabledBreakpointPixmap()
{
  static QPixmap pixmap=KIcon("script-error").pixmap(QSize(22,22), QIcon::Disabled, QIcon::Off);
  return &pixmap;
}

const QPixmap* BreakpointController::executionPointPixmap()
{
  static QPixmap pixmap=KIcon("go-next").pixmap(QSize(22,22), QIcon::Normal, QIcon::Off);
  return &pixmap;
}

#if 0
int BreakpointController::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return Last + 1;
}

QVariant BreakpointController::data(const QModelIndex & index, int role) const
{
    Breakpoint* breakpoint = breakpointForIndex(index);
    if (!breakpoint)
        return QVariant();

    switch (index.column()) {
        case Enable:
            switch (role) {
                case Qt::CheckStateRole:
                case Qt::EditRole:
                    return breakpoint->isEnabled();
            }
            break;

        case Type:
            switch (role) {
                case Qt::DisplayRole: {
                    QString displayType = breakpoint->displayType();
                    if (breakpoint->isTemporary())
                        displayType = i18n(" temporary");
                    if (breakpoint->isHardwareBP())
                        displayType += i18n(" hw");
                    return displayType;
                }
            }
            break;

        case Status:
            switch (role) {
                case Qt::DisplayRole:
                    return breakpoint->statusDisplay(m_activeFlag);
            }
            break;

        case Location:
            switch (role) {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    return breakpoint->location();
            }
            break;

        case Condition:
            switch (role) {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    return breakpoint->conditional();
            }
            break;

        case IgnoreCount:
            switch (role) {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    return breakpoint->ignoreCount();
            }
            break;

        case Hits:
            switch (role) {
                case Qt::DisplayRole:
                    return breakpoint->hits();
            }
            break;

        case Tracing:
            switch (role) {
                case Qt::DisplayRole:
                    return breakpoint->tracingEnabled() ? i18n("Enabled") : i18n("Disabled");
            }
            break;
    }

    return QVariant();
}

Qt::ItemFlags BreakpointController::flags(const QModelIndex & index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable;

    flags |= Qt::ItemIsEnabled;

    if (index.column() == Enable ||
        index.column() == Location ||
        index.column() == Condition ||
        index.column() == IgnoreCount)
        flags |= Qt::ItemIsEditable;

    return flags;
}

QVariant BreakpointController::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (section) {
        case Enable:
            break;//return i18n("Enabled");
        case Type:
            return i18n("Type");
        case Status:
            return i18n("Status");
        case Location:
            return i18n("Location");
        case Condition:
            return i18n("Condition");
        case IgnoreCount:
            return i18n("Ignore Count");
        case Hits:
            return i18n("Hits");
        case Tracing:
            return i18n("Tracing");
    }

    return QVariant();
}

QModelIndex BreakpointController::index(int row, int column, const QModelIndex & parent) const
{
    if (row < 0 || column < 0 || column > Last)
        return QModelIndex();

    if (row >= m_breakpoints.count())
        return QModelIndex();

    return createIndex(row, column, m_breakpoints.at(row));
}

QModelIndex BreakpointController::parent(const QModelIndex & index) const
{
    Q_UNUSED(index);

    return QModelIndex();
}

int BreakpointController::rowCount(const QModelIndex & parent) const
{
    if (!parent.isValid())
        return m_breakpoints.count();

    return 0;
}

bool BreakpointController::setData(const QModelIndex & index, const QVariant & value, int role)
{
    Breakpoint* bp = breakpointForIndex(index);
    if (!bp)
        return false;

    bool changed = false;

    switch (role) {
        case Qt::EditRole:
            switch (index.column()) {
                case Location:
                    if (bp->location() != value.toString())
                    {
                        // GDB does not allow to change location of
                        // an existing breakpoint. So, need to remove old
                        // breakpoint and add another.

                        // Announce to editor that breakpoit at its
                        // current location is dying.
                        bp->setActionDie();
                        adjustMark(bp, false);

                        // However, we don't want the line in breakpoint
                        // widget to disappear and appear again.

                        // Emit delete command. This won't resync breakpoint
                        // table (unlike clearBreakpoint), so we won't have
                        // nasty effect where line in the table first disappears
                        // and then appears again, and won't have internal issues
                        // as well.
                        if (!controller()->stateIsOn(s_dbgNotStarted))
                            controller()->addCommand(BreakDelete, bp->dbgRemoveCommand().toLatin1());

                        // Now add new breakpoint in gdb. It will correspond to
                        // the same 'Breakpoint' and 'BreakpointRow' objects in
                        // KDevelop is the previous, deleted, breakpoint.

                        // Note: clears 'actionDie' implicitly.
                        bp->setActionAdd(true);
                        bp->setLocation(value.toString());
                        adjustMark(bp, true);
                        changed = true;
                    }
                    break;

                case Condition:
                    bp->setConditional(value.toString());
                    changed = true;
                    break;

                case IgnoreCount:
                    bp->setIgnoreCount(value.toInt());
                    changed = true;
                    break;

                case Enable:
                    bp->setEnabled(value.toBool());
                    changed = true;
                    break;
            }
            break;
    }

    if (changed) {
        bp->setActionModify(true);
        bp->sendToGdb();

        emit dataChanged(index, index);
    }

    return changed;
}

Breakpoint * BreakpointController::breakpointForIndex(const QModelIndex & index) const
{
    if (!index.isValid())
        return 0;

    return static_cast<Breakpoint*>(index.internalPointer());
}

#endif

GDBController * BreakpointController::controller() const
{
    return static_cast<GDBController*>(const_cast<QObject*>(QObject::parent()));
}



void BreakpointController::slotEvent(event_t e)
{
    if (e == program_running ||
        e == program_exited ||
        e == debugger_exited)
    {
        clearExecutionPoint();
    }

    switch(e)
    {
    case program_state_changed:
        {
            universe_->update();
        }

    case connected_to_program:
        {
            kDebug(9012) << "connected to program";
            universe_->sendToGDB();
            #if 0
            foreach (Breakpoint* bp, breakpoints())
            {
                if ( (bp->dbgId() == -1 ||  bp->isPending())
                        && !bp->isDbgProcessing()
                        && bp->isValid())
                {
                    bp->sendToGdb();
                }
            }
            #endif
            break;
        }
    case program_exited:
        {
            #if 0
            foreach (Breakpoint* b, breakpoints())
            {
                b->applicationExited();
            }
            #endif
        }
    case debugger_exited:
        universe_->markOut();
        break;
    default:
        ;
    }
}


const QList< Breakpoint * > & BreakpointController::breakpoints() const
{
    return m_breakpoints;
}

void BreakpointController::handleBreakpointList(const GDBMI::ResultRecord& r)
{
#if 0
    m_activeFlag++;

    const GDBMI::Value& blist = r["BreakpointTable"]["body"];

    for(int i = 0, e = blist.size(); i != e; ++i)
    {
        const GDBMI::Value& b = blist[i];

        int id = b["number"].literal().toInt();
        Breakpoint* bp = findBreakpointById(id);
        if (bp)
        {
            bp->setActive(m_activeFlag, id);
            bp->setHits(b["times"].toInt());
            if (b.hasField("ignore"))
                bp->setIgnoreCount(b["ignore"].toInt());
            else
                bp->setIgnoreCount(0);
            if (b.hasField("cond"))
                bp->setConditional(b["cond"].literal());
            else
                bp->setConditional(QString::null);

            // TODO: make the above functions do this instead
            bp->notifyModified();
        }
        else
        {
            // It's a breakpoint added outside, most probably
            // via gdb console. Add it now.
            QString type = b["type"].literal();

            if (type == "breakpoint" || type == "hw breakpoint")
            {
                if (b.hasField("fullname") && b.hasField("line"))
                {
                    Breakpoint* bp = new FilePosBreakpoint(this,
                        b["fullname"].literal(),
                        b["line"].literal().toInt());

                    bp->setActive(m_activeFlag, id);
                    bp->setActionAdd(false);
                    bp->setPending(false);

                    addBreakpoint(bp);
                }
            }

        }
    }

    // Remove any inactive breakpoints.
    foreach (Breakpoint* breakpoint, breakpoints())
    {
        if (!(breakpoint->isActive(m_activeFlag)))
        {
            // FIXME: need to review is this happens for
            // as-yet unset breakpoint.
            breakpoint->removedInGdb();
        }
    }
#endif
}

FilePosBreakpoint * BreakpointController::findBreakpoint(const QString & file, int line) const
{
    foreach (Breakpoint* bp, breakpoints())
        if (FilePosBreakpoint* fbp = qobject_cast<FilePosBreakpoint*>(bp))
            if (fbp->fileName() == file && fbp->lineNum() == line)
                return fbp;

    return 0;
}

Watchpoint * BreakpointController::findWatchpoint(const QString & variableName) const
{
    foreach (Breakpoint* bp, breakpoints())
        if (Watchpoint* wp = qobject_cast<Watchpoint*>(bp))
            if (wp->varName() == variableName)
                return wp;

    return 0;
}

Breakpoint* BreakpointController::findBreakpointById(int id) const
{
    foreach (Breakpoint* bp, breakpoints())
        if (bp->dbgId() == id)
            return bp;

    return 0;
}

#if 0
void BreakpointController::savePartialProjectSession(QDomElement* el)
{
    /*QDomDocument domDoc = el->ownerDocument();
    if (domDoc.isNull())
        return;

    QDomElement breakpointListEl = domDoc.createElement("breakpointList");
    for ( int row = 0; row < m_table->numRows(); row++ )
    {
        BreakpointTableRow* btr =
            (BreakpointTableRow *) m_table->item(row, Control);
        Breakpoint* bp = btr->breakpoint();

        QDomElement breakpointEl =
            domDoc.createElement("breakpoint"+QString::number(row));

        breakpointEl.setAttribute("type", bp->type());
        breakpointEl.setAttribute("location", bp->location(false));
        breakpointEl.setAttribute("enabled", bp->isEnabled());
        breakpointEl.setAttribute("condition", bp->conditional());
        breakpointEl.setAttribute("tracingEnabled",
                                  QString::number(bp->tracingEnabled()));
        breakpointEl.setAttribute("traceFormatStringEnabled",
                                  QString::number(bp->traceFormatStringEnabled()));
        breakpointEl.setAttribute("tracingFormatString",
                                  bp->traceFormatString());

        QDomElement tracedExpressions =
            domDoc.createElement("tracedExpressions");

        QStringList::const_iterator i, e;
        for(i = bp->tracedExpressions().begin(),
                e = bp->tracedExpressions().end();
            i != e; ++i)
        {
            QDomElement expr = domDoc.createElement("expression");
            expr.setAttribute("value", *i);
            tracedExpressions.appendChild(expr);
        }

        breakpointEl.appendChild(tracedExpressions);

        breakpointListEl.appendChild(breakpointEl);
    }

    if (!breakpointListEl.isNull())
        el->appendChild(breakpointListEl);*/
}

/***************************************************************************/

void BreakpointController::restorePartialProjectSession(const QDomElement* el)
{
    /** Eventually, would be best to make each breakpoint type handle loading/
        saving it's data. The only problem is that on load, we need to allocate
        with new different objects, depending on type, and that requires some
        kind of global registry. Gotta find out if a solution for that exists in
        KDE (Boost.Serialization is too much dependency, and rolling my own is
        boring).
    */
    /*QDomElement breakpointListEl = el->namedItem("breakpointList").toElement();
    if (!breakpointListEl.isNull())
    {
        QDomElement breakpointEl;
        for (breakpointEl = breakpointListEl.firstChild().toElement();
                !breakpointEl.isNull();
                breakpointEl = breakpointEl.nextSibling().toElement())
        {
            Breakpoint* bp=0;
            BP_TYPES type = (BP_TYPES) breakpointEl.attribute( "type", "0").toInt();
            switch (type)
            {
            case BP_TYPE_FilePos:
            {
                bp = new FilePosBreakpoint();
                break;
            }
            case BP_TYPE_Watchpoint:
            {
                bp = new Watchpoint("");
                break;
            }
            default:
                break;
            }

            // Common settings for any type of breakpoint
            if (bp)
            {
                bp->setLocation(breakpointEl.attribute( "location", ""));
                if (type == BP_TYPE_Watchpoint)
                {
                    bp->setEnabled(false);
                }
                else
                {
                    bp->setEnabled(
                        breakpointEl.attribute( "enabled", "1").toInt());
                }
                bp->setConditional(breakpointEl.attribute( "condition", ""));

                bp->setTracingEnabled(
                    breakpointEl.attribute("tracingEnabled", "0").toInt());
                bp->setTraceFormatString(
                    breakpointEl.attribute("tracingFormatString", ""));
                bp->setTraceFormatStringEnabled(
                    breakpointEl.attribute("traceFormatStringEnabled", "0")
                    .toInt());

                QDomNode tracedExpr =
                    breakpointEl.namedItem("tracedExpressions");

                if (!tracedExpr.isNull())
                {
                    QStringList l;

                    for(QDomNode c = tracedExpr.firstChild(); !c.isNull();
                        c = c.nextSibling())
                    {
                        QDomElement el = c.toElement();
                        l.push_back(el.attribute("value", ""));
                    }
                    bp->setTracedExpressions(l);
                }

                // Now add the breakpoint. Don't try to check if
                // breakpoint already exists.
                // It's easy to check that breakpoint on the same
                // line already exists, but it might have different condition,
                // and checking conditions for equality is too complex thing.
                // And anyway, it's will be suprising of realoading a project
                // changes the set of breakpoints.
                addBreakpoint(bp);
            }
        }
    }*/
}
#endif

Watchpoint* BreakpointController::findWatchpointByAddress(quint64 address) const
{
    foreach (Breakpoint* breakpoint, breakpoints())
        if (Watchpoint* w = qobject_cast<Watchpoint*>(breakpoint))
            if (w->address() == address)
                return w;

    return false;
}

Breakpoint* BreakpointController::addBreakpoint(Breakpoint *bp)
{
    beginInsertRows(QModelIndex(), m_breakpoints.count(), m_breakpoints.count());

    m_breakpoints.append(bp);

    endInsertRows();

    connect(bp, SIGNAL(modified(Breakpoint*)),
            this, SLOT(slotBreakpointModified(Breakpoint*)));
    connect(bp, SIGNAL(enabledChanged(Breakpoint*)),
            this, SLOT(slotBreakpointEnabledChanged(Breakpoint*)));

    bp->sendToGdb();

    adjustMark(bp, true);

    return bp;
}

void BreakpointController::adjustMark(Breakpoint* bp, bool add)
{
    if (FilePosBreakpoint* fbp = qobject_cast<FilePosBreakpoint*>(bp)) {
        if (KDevelop::IDocument* document = KDevelop::ICore::self()->documentController()->documentForUrl(KUrl(fbp->fileName()))) {
            if (MarkInterface *iface = dynamic_cast<MarkInterface*>(document->textDocument())) {
                // Temporarily block so we don't get confused by receiving extra
                // marksChanged signals
                document->textDocument()->blockSignals(true);

                iface->removeMark( fbp->lineNum() - 1, BreakpointMark | ActiveBreakpointMark | ReachedBreakpointMark | DisabledBreakpointMark );

                if (add) {
                    uint markType = BreakpointMark;
                    if( !bp->isPending() )
                        markType |= ActiveBreakpointMark;
                    if( !bp->isEnabled() )
                        markType |= DisabledBreakpointMark;
                    iface->addMark( fbp->lineNum() - 1, markType );
                }

                document->textDocument()->blockSignals(false);
            }
        }
    }
}


void BreakpointController::removeBreakpoint(Breakpoint* bp)
{
    if (!bp)
        return;

    int row = m_breakpoints.indexOf(bp);
    Q_ASSERT(row != -1);
    if (row == -1)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_breakpoints.removeAt(row);
    endRemoveRows();

    adjustMark(bp, false);

    bp->remove();
}

#if 0
QModelIndex BreakpointController::indexForBreakpoint(Breakpoint * breakpoint, int column) const
{
    if (!breakpoint)
        return QModelIndex();

    int row = m_breakpoints.indexOf(breakpoint);
    if (row == -1)
        return QModelIndex();

    return createIndex(row, column, breakpoint);
}
#endif

void BreakpointController::removeAllBreakpoints()
{
    foreach (Breakpoint* breakpoint, breakpoints())
        breakpoint->remove();

    m_breakpoints.clear();
    reset();
}

void BreakpointController::slotBreakpointModified(Breakpoint* b)
{
    if (b->isActionDie())
    {
        // Breakpoint was deleted, kill the table row.
        removeBreakpoint(b);
    }
    else
    {
#if 0
        emit dataChanged(indexForBreakpoint(b, 0), indexForBreakpoint(b, Last));
#endif
    }
}

void BreakpointController::slotBreakpointEnabledChanged(Breakpoint * b)
{
    adjustMark(b, true);
}

const int NewBreakpoint::enable_column;
const int NewBreakpoint::state_column;
const int NewBreakpoint::type_column;
const int NewBreakpoint::location_column;
const int NewBreakpoint::condition_column;

const char *NewBreakpoint::string_kinds[last_breakpoint_kind] = {
    "Code",
    "Write",
    "Read",
    "Access"
};


#include "breakpointcontroller.moc"