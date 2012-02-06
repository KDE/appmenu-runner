
#include <QString>
#include <QMap>

namespace Oxygen
{

    //! generate translations between kde icon names and gtk icons
    class GtkIcons : public QMap<QString, QString>
    {
        public:

        //! constructor
        GtkIcons( void );
    };
}