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

#include "appmenu.h"
#include "app_menu.h"
#include "dbus_menu.h"
#include "dbusmenutypes_p.h"

#include <QMenu>
#include <QApplication>

#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KWindowSystem>

#include <qdbusmetatype.h>

#define REGISTRAR_PATH "/com/canonical/AppMenu/Registrar"
#define REGISTRAR_SERVICE "com.canonical.AppMenu.Registrar"

AppMenu::AppMenu(QObject *parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args)
    , m_activeWid(0)
    , m_dbusMenu(0)
    , m_appMenu(0)
{
    Q_UNUSED(args);
    DBusMenuTypes_register();

    setObjectName("appmenu");

    getTopLevelWindows();

    m_appMenu = new com::canonical::AppMenu::Registrar(REGISTRAR_SERVICE, REGISTRAR_PATH, QDBusConnection::sessionBus());

    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(activeWindowChanged(WId)));

    activeWindowChanged(KWindowSystem::self()->activeWindow());
}

AppMenu::~AppMenu()
{
    delete m_dbusMenu;
    delete m_appMenu;
}

void AppMenu::activeWindowChanged(WId wid)
{
    if (m_topWindows.contains(wid)) {
        return;
    }

    m_activeWid = wid;
}

void AppMenu::match(Plasma::RunnerContext &context)
{
    const QString term = context.query().toLower();
    if (term.length() < 3) {
        return;
    }

    delete m_dbusMenu;
    m_dbusMenu = 0;

    QPair <QString, QString> dbusInfo = getMenuForActiveWindow();
    m_dbusMenu = new com::canonical::dbusmenu(dbusInfo.first, dbusInfo.second, QDBusConnection::sessionBus());

    qDebug() << "Getting Layout";

    const DBusMenuLayoutItem topItem = getTopLevelItem();

    if (topItem.id == -1) {
        return;
    }

    QString path;
    MatchList results;
    Q_FOREACH(const DBusMenuLayoutItem &topLevel, topItem.children) {
        path.append(topLevel.properties.value("label").toString().remove("_"));
        path.append(" > ");

        inspectForMatches(topLevel, term, results, path);

        path.clear();
    }

    context.addMatches(term, results);

}

void AppMenu::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

    QDBusVariant empty;
    empty.setVariant(QVariant::fromValue<QString>(QString()));
    QDBusPendingReply <void > reply = m_dbusMenu->Event(match.data().toInt(), "clicked", empty, QDateTime::currentDateTime().toTime_t());

    qDebug() << reply.isError();
    qDebug() << reply.error().message();
    qDebug() << reply.error().name();
}

const QPair< QString, QString > AppMenu::getMenuForActiveWindow() const
{
    qDebug() << "Getting menu for widow: " << m_activeWid;
    QDBusPendingReply <QString, QDBusObjectPath > reply =  m_appMenu->GetMenuForWindow(m_activeWid);
    reply.waitForFinished();

    QPair<QString, QString> dbusInfo;
    if (reply.isError()) {
        handleDBusError(reply.error());
        return dbusInfo;
    }

    dbusInfo.first = reply.argumentAt<0>();
    dbusInfo.second = reply.argumentAt<1>().path();

    return dbusInfo;
}

const DBusMenuLayoutItem AppMenu::getTopLevelItem() const
{
    QDBusPendingReply <uint, DBusMenuLayoutItem > topItems =  m_dbusMenu->GetLayout(0, -1, QStringList());
    topItems.waitForFinished();

    if (topItems.isError()) {
        handleDBusError(topItems.error());
        DBusMenuLayoutItem errorItem;
        errorItem.id = -1;
        return errorItem;
    }

    return topItems.argumentAt<1>();
}

void AppMenu::inspectForMatches(const DBusMenuLayoutItem& topItem, QString query, MatchList& matchList, QString &path)
{
    QString label;
    Q_FOREACH(const DBusMenuLayoutItem &item, topItem.children) {
        label = item.properties.value("label").toString().remove("_");

        if (!item.children.isEmpty()) {
            QString subPath(path);
            subPath.append(label);
            subPath.append(" > ");

            inspectForMatches(item, query, matchList, subPath);
            subPath.clear();
            continue;
        }

        if (label.contains(query, Qt::CaseInsensitive)) {
            addMatch(item, matchList, path);
        }
    }
}

void AppMenu::addMatch(const DBusMenuLayoutItem& item, MatchList& matchList, QString &path)
{
    QString text = item.properties.value("label").toString().remove("_");
    QString subText = QString();
    subText.append(path);
    subText.append(text);

    Plasma::QueryMatch match(this);
    match.setText(text);
    match.setSubtext(subText);
    match.setId(QString::number(item.id));

    match.setData(QVariant::fromValue(item.id));

    if (item.properties.contains("icon-name")) {
        match.setIcon(QIcon::fromTheme(item.properties["icon-name"].toString()));
    }

    matchList.append(match);
}

void AppMenu::getTopLevelWindows()
{
    QApplication *app = qobject_cast< QApplication* >(qApp);
    QWidgetList topWidgets = app->topLevelWidgets();

    Q_FOREACH(const QWidget *window, topWidgets) {
        if (window->isWindow()) {
            m_topWindows.append(window->winId());
        }
    }
}

void AppMenu::handleDBusError(const QDBusError& error) const
{
    qDebug() << error.name();
    qDebug() << error.message();
}

#include "appmenu.moc"
