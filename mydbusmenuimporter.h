
#include "icons.h"

#include <dbusmenuimporter.h>

#include <KIcon>
#include <kiconloader.h>

class MyDBusMenuImporter : public DBusMenuImporter
{
public:
    MyDBusMenuImporter(const QString &service, Oxygen::GtkIcons *icons, const QString &path, QObject *parent)
    : DBusMenuImporter(service, path, parent)
    , mService(service)
    , mPath(path)
    {
        mIcons = icons;
    }

    QString service() const { return mService; }
    QString path() const { return mPath; }


protected:
    virtual QIcon iconForName(const QString &name)
    {
        KIcon icon;

        if( mIcons->contains( name ) )
        {
            icon =  KIcon( mIcons->value( name ) );
        }
        else if( !KIconLoader::global()->iconPath( name , 1 , true ).isNull() )
        {
            icon = KIcon( name );
        }

        return icon;
    }

private:
    Oxygen::GtkIcons *mIcons;
    QString mService;
    QString mPath;
};