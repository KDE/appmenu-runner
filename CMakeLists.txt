project(plasma-appmenu)

# Find the required Libaries
find_package(KDE4 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUSMENUQT REQUIRED dbusmenu-qt>=0.6.0)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

add_subdirectory(src)