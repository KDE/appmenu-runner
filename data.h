#include <QDBusObjectPath>

struct MenuInfo
{
    MenuInfo()
    : winId(0)
    , path("/")
    {}

    uint winId;
    QString service;
    QDBusObjectPath path;
};
Q_DECLARE_METATYPE(MenuInfo)

typedef QList<MenuInfo> MenuInfoList;
Q_DECLARE_METATYPE(MenuInfoList)


