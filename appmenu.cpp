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

QDBusArgument& operator<<(QDBusArgument& argument, const MenuInfo& info)
{
    argument.beginStructure();
    argument << info.winId << info.service << info.path;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, MenuInfo& info)
{
    argument.beginStructure();
    argument >> info.winId >> info.service >> info.path;
    argument.endStructure();
    return argument;
}

AppMenu::AppMenu(QObject *parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args)
    , m_dbusMenu(0)
{
    Q_UNUSED(args);
    qDBusRegisterMetaType <MenuInfo>();
    qDBusRegisterMetaType <MenuInfoList>();
    DBusMenuTypes_register();

    setObjectName("appmenu");

    getTopLevelWindows();

    m_appMenu = new com::canonical::AppMenu::Registrar("com.canonical.AppMenu.Registrar", "/com/canonical/AppMenu/Registrar", QDBusConnection::sessionBus());

    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(activeWindowChanged(WId)));

    activeWindowChanged(KWindowSystem::self()->activeWindow());
}

AppMenu::~AppMenu()
{
}

void AppMenu::activeWindowChanged(WId wid)
{
    qDebug() << "New window: " << wid;

    if (m_topWindows.contains(wid) || wid == 0) {
        return;
    }

    qDebug() << "Active window changed: " << wid;
    m_activeWid = wid;
}

void AppMenu::match(Plasma::RunnerContext &context)
{
    delete m_dbusMenu;
    m_dbusMenu = 0;

    const QString term = context.query().toLower();
    if (term.length() < 3) {
        return;
    }

    qDebug() << "Getting menu for widow: " << m_activeWid;
    QDBusPendingReply <QString, QDBusObjectPath > reply =  m_appMenu->GetMenuForWindow(m_activeWid);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << reply.error().message();
        qDebug() << reply.error().name();
        return;
    }

    qDebug() << reply.argumentAt<0>();
    m_dbusMenu = new com::canonical::dbusmenu(reply.argumentAt<0>(), reply.argumentAt<1>().path(), QDBusConnection::sessionBus());
    qDebug() << "Getting Layout";
    QDBusPendingReply <uint, DBusMenuLayoutItem > menuItems =  m_dbusMenu->GetLayout(0, -1, QStringList());
    menuItems.waitForFinished();

    const DBusMenuLayoutItem topItem =  menuItems.argumentAt<1>();

    MatchList results;
    QString path;
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
    qDebug() << match.id();
    QDBusPendingReply <void > reply = m_dbusMenu->Event(match.data().toInt(), "clicked", empty, QDateTime::currentDateTime().toTime_t());

    qDebug() << reply.isError();
    qDebug() << reply.error().message();
    qDebug() << reply.error().name();
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

#include "appmenu.moc"
