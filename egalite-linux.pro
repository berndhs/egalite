
PROJECT = egalite

CONFIG += gencert

CONFIG += lib_pkgconfig

PKGCONFIG += qxmpp

include ("egalite-common.pri")

target.path = /usr/local/bin
INSTALLS += target

icon.files = harmattan/egalite.png
icon.path = /usr/local/share/pixmaps
INSTALLS += icon

desktop.files = harmattan/egalite.desktop
desktop.path = /usr/local/share/applications
INSTALLS += desktop
