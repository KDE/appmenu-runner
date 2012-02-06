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
    , m_icons(new Oxygen::GtkIcons())
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
    const QString term = context.query().toLower();
    if (term.length() < 3) {
        return;
    }

    QDBusPendingReply <QString, QDBusObjectPath > reply =  m_appMenu->GetMenuForWindow(m_activeWid);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << reply.error().message();
        qDebug() << reply.error().message();
        return;
    }

    com::canonical::dbusmenu *menu = new com::canonical::dbusmenu(reply.argumentAt<0>(), reply.argumentAt<1>().path(), QDBusConnection::sessionBus());
    QDBusPendingReply <uint, DBusMenuLayoutItem > menuItems =  menu->GetLayout(0, -1, QStringList());
    menuItems.waitForFinished();

    const DBusMenuLayoutItem item =  menuItems.argumentAt<1>();

    MatchList results;
    inspectForMatches(item, term, results);

    context.addMatches(term, results);
}

void AppMenu::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)
}

void AppMenu::inspectForMatches(const DBusMenuLayoutItem& topItem, QString query, MatchList& result)
{
    if (topItem.children.isEmpty()) {
        return;
    }

    QString label;
    Q_FOREACH(const DBusMenuLayoutItem &item, topItem.children) {
        if (!item.children.isEmpty()) {
            inspectForMatches(item, query, result);
            continue;
        }

        label = item.properties.value("label").toString().remove("_");
        if (label.contains(query, Qt::CaseInsensitive)) {
            addMatch(item, result);
        }
    }
}

void AppMenu::addMatch(const DBusMenuLayoutItem& item, MatchList& result)
{
    Plasma::QueryMatch match(this);

    match.setText(item.properties.value("label").toString().remove("_"));
    match.setId(item.properties["id"].toString());

    if (item.properties.contains("icon-name")) {
        match.setIcon(QIcon::fromTheme(item.properties["icon-name"].toString()));
    }

    result.append(match);
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

    qDebug() << "Top level windows";
    qDebug() << m_topWindows;
}

#include "appmenu.moc"
