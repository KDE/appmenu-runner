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
    , m_icons(new Oxygen::GtkIcons())
    , m_dbusMenu(0)
{
    Q_UNUSED(args);
    qDBusRegisterMetaType <MenuInfo>();
    qDBusRegisterMetaType <MenuInfoList>();
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
    delete m_icons;
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

    qDebug() << "Getting menu for widow: " << m_activeWid;
    QDBusPendingReply <QString, QDBusObjectPath > reply =  m_appMenu->GetMenuForWindow(m_activeWid);
    reply.waitForFinished();
    if (reply.isError()) {
        handleDBusError(reply.error());
        return;
    }

    qDebug() << reply.argumentAt<0>();
    m_dbusMenu = new com::canonical::dbusmenu(reply.argumentAt<0>(), reply.argumentAt<1>().path(), QDBusConnection::sessionBus());
    qDebug() << "Getting Layout";
    QDBusPendingReply <uint, DBusMenuLayoutItem > menuItems =  m_dbusMenu->GetLayout(0, -1, QStringList());
    menuItems.waitForFinished();

    if (menuItems.isError()) {
        handleDBusError(reply.error());
        return;
    }

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

void AppMenu::handleDBusError(const QDBusError& error) const
{
    qDebug() << error.name();
    qDebug() << error.message();
}

#include "appmenu.moc"
