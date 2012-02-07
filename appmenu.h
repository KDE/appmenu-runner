
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
