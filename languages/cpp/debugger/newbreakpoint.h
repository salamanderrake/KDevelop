/* This file is part of the KDE project
   Copyright (C) 2002 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
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

#ifndef NEWBREAKPOINT_H
#define NEWBREAKPOINT_H

#include "util/treeitem.h"
#include <QSet>

class KConfigGroup;
namespace GDBMI { class Value; class ResultRecord; }
namespace GDBDebugger
{
    
class GDBController;

class NewBreakpoint : public TreeItem
{
public:
    enum kind_t { code_breakpoint = 0, write_breakpoint, read_breakpoint,
                  access_breakpoint, last_breakpoint_kind };

    NewBreakpoint(TreeModel *model, TreeItem *parent, GDBController* controller,
                  kind_t kind);
    NewBreakpoint(TreeModel *model, TreeItem *parent, GDBController* controller,
                  const KConfigGroup& config);

    /* This constructor creates a "please enter location" item, that will
       turn into real breakpoint when user types something.  */
    NewBreakpoint(TreeModel *model, TreeItem *parent,
                  GDBController* controller);

    int id() const { return id_; }
    void update(const GDBMI::Value &b);
    void fetchMoreChildren() {}

    /* Mark this breakpoint as no longer inserted, due to GDB
       no longer running.  */
    void markOut();

    void setColumn(int index, const QVariant& value);
    void setDeleted();

    void sendToGDBMaybe();
    int hitCount() const;

    QVariant data(int column, int role) const;

    bool pending() const { return pending_; }
    bool dirty() const { return !dirty_.empty(); }
    
    void save(KConfigGroup& config);

    static const int enable_column = 0;
    static const int state_column = 1;
    static const int type_column = 2;
    static const int location_column = 3;
    static const int condition_column = 4;
        
private:
    void handleDeleted(const GDBMI::ResultRecord &v);
    void handleInserted(const GDBMI::ResultRecord &r);
    void handleEnabledOrDisabled(const GDBMI::ResultRecord &r);
    void handleConditionChanged(const GDBMI::ResultRecord &r);
    void handleAddressComputed(const GDBMI::ResultRecord &r);

    friend class Breakpoints;
    void setLocation(const QString& location);

    int id_;
    bool enabled_;
    QSet<int> dirty_;
    QSet<int> errors_;
    GDBController* controller_;
    bool deleted_;
    int hitCount_;
    kind_t kind_;
    /* The GDB 'pending' flag.  */
    bool pending_;
    /* For watchpoints, the address it is set at.  */
    QString address_;
    bool pleaseEnterLocation_;

    static const char *string_kinds[last_breakpoint_kind];
};

}
#endif