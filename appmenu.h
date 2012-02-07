/***************************************************************************
 *   Copyright (C) 2012 Alejandro Fiestas Olivares <afiestas@kde.org>      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU Library General Public License as         *
 * published by the Free Software Foundation; either version 2 of the      *
 * License, or (at your option) any later version.                         *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * Library General Public License for more details.                        *
 *                                                                         *
 * You should have received a copy of the GNU Library General Public       *
 * License along with this program; see the file COPYING.  If not, write   *
 * to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, *
 * Boston, MA 02110-1301, USA.                                             *
 ***************************************************************************/

#ifndef APPMENU_H
#define APPMENU_H

#include "app_menu.h"
#include "dbus_menu.h"
#include "dbusmenutypes_p.h"

#include <QMenu>

#include <plasma/abstractrunner.h>

typedef QList<Plasma::QueryMatch> MatchList;

class AppMenu : public Plasma::AbstractRunner
{
    Q_OBJECT

    public:
    // Basic Create/Destroy
    AppMenu( QObject *parent, const QVariantList& args );
    ~AppMenu();

    void match(Plasma::RunnerContext &context);
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match);

    public Q_SLOTS:
        void activeWindowChanged(WId wid);

    private:
        void inspectForMatches(const DBusMenuLayoutItem &topItem, QString query, MatchList &matchList, QString &path);
        void getTopLevelWindows();
        void addMatch(const DBusMenuLayoutItem& item, MatchList& matchList, QString& path);

    private:
        WId m_activeWid;
        QList<WId> m_topWindows;
        com::canonical::dbusmenu *m_dbusMenu;
        com::canonical::AppMenu::Registrar *m_appMenu;
};

// This is the command that links your applet to the .desktop file
K_EXPORT_PLASMA_RUNNER(appmenu, AppMenu)
#endif
