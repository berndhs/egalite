
PROJECT = egalite

CONFIG -= gencert

INCLUDEPATH += /opt/qxmpp/include/qxmpp

LIBS += -L/opt/qxmpp/lib

include ("egalite-common.pri")

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog


contains(MEEGO_EDITION,harmattan) {
    target.path = /opt/egalite/bin
    INSTALLS += target
}

contains(MEEGO_EDITION,harmattan) {
    icon.files = harmattan/egalite.png
    icon.path = /usr/share/icons/hicolor/64x64/apps
    INSTALLS += icon
}

contains(MEEGO_EDITION,harmattan) {
    desktop.files = harmattan/egalite.desktop
    desktop.path = /usr/share/applications
    INSTALLS += desktop
}
