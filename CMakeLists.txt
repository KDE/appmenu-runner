project(plasma-appmenu)

# Find the required Libaries
find_package(KDE4 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUSMENUQT REQUIRED dbusmenu-qt>=0.6.0)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   ${DBUSMENUQT_INCLUDE_DIRS}
   )

# We add our source code here
set(appmenu_SRCS appmenu.cpp dbusmenutypes_p.cpp dbusmenushortcut_p.cpp)

set(appmenu_xml com.canonical.AppMenu.Registrar.xml)
set_source_files_properties(${appmenu_xml} PROPERTIES INCLUDE "data.h")
qt4_add_dbus_interface(appmenu_SRCS ${appmenu_xml} app_menu)

set(dbusmenu_xml com.canonical.dbusmenu.xml)
set_source_files_properties(${dbusmenu_xml} PROPERTIES INCLUDE "dbusmenutypes_p.h")
qt4_add_dbus_interface(appmenu_SRCS ${dbusmenu_xml} dbus_menu)

kde4_add_plugin(krunner_appmenu ${appmenu_SRCS})

target_link_libraries(krunner_appmenu
                      ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${DBUSMENUQT_LIBRARIES})

install(TARGETS krunner_appmenu
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES appmenu.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})

