
# dchat 
#
#//
#//  Copyright (C) 2010,2011 - Bernd H Stramm 
#//
#// This program is distributed under the terms of 
#// the GNU General Public License version 3 
#//
#// This software is distributed in the hope that it will be useful, 
#// but WITHOUT ANY WARRANTY; without even the implied warranty 
#// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
#//


TEMPLATE = app
MAKEFILE = Makefile
unix {
  CONFIG += debug_and_release
  CONFIG += crypto
  CONFIG += link_pkgconfig
  gencert {
    PKGCONFIG += qca2
  }
}
win32 {
  CONFIG += debug_and_release
  CONFIG += link_prl
}
CONFIG(debug,debug|release) {
  TARGET = bin/egalite_d
  BUILD_SUB = bld_debug
}
CONFIG(release,debug|release) {
  TARGET = bin/egalite
  BUILD_SUB = bld_release
}
TRANS_DIR = translate
TRANSLATIONS += $$TRANS_DIR/egalite_fr.ts $$TRANS_DIR/egalite_de.ts
message ("translations in $$TRANS_DIR/")

CODEFORTR = UTF-8

message ("generating MAKEFILE as $$MAKEFILE")
QT += core gui network xml xmlpatterns sql webkit declarative


DEFINES += DELIBERATE_QTM1=$$QT_MAJOR_VERSION
DEFINES += DELIBERATE_QTM2=$$QT_MINOR_VERSION
DEFINES += DELIBERATE_QTP=$$QT_PATCH_VERSION

gencert {
  DEFINES += EGALITE_GENCERT=1
} else {
  DEFINES += EGALITE_GENCERT=0
}

INCLUDEPATH += ./include


unix {
  message ("Applying Unix settings")
  !include ( options.pri ) {
    LIBS += -L./qxmpp/lib -lqxmpp 
    DEFINES += DO_AUDIO=0
    message ("No options.pri, using default $$INCLUDEPATH")
  } else {
    DEFINES += DO_AUDIO=$$DO_AUDIO
    LIBX += -L./qxmpp/lib -lqxmpp 
  }
  message ("include path: $$INCLUDEPATH")
  DEFINES += DELIBERATE_DEBUG=1
#  INCLUDEPATH += /usr/include/QtCrypto
#  LIBS += -lqca
#
# use the code below if QXmpp is installed in /usr/local
#  INCLUDEPATH += /usr/local/include/qxmpp
#  exists ("/usr/local/lib64") {
#    LIBS += -L/usr/local/lib64 -l$$QXMPP_NAME
#  } else { 
#    LIBS += -L/usr/local/lib -lQ$$QXMPP_NAME
#  }
#
}

win32 {
  message ("Applying Windows 32 bit mingw settings")
  CONFIG += build_x86
  INCLUDEPATH += ../../software/qca/qca-2.0.2/include/QtCrypto
  INCLUDEPATH += ../qxmpp-0.2.0/src
  QTDIR = C:/Qt/2010.05/qt
  LIBS += -L'd:/bernd/local/OpenSSL-Win32'
  LIBS += -L'd:/bernd/local/OpenSSL-Win32/lib'
  CONFIG(debug, debug|release) {
    LIBS += ../qxmpp-0.2.0/lib/libqxmpp_d.a
    LIBS += $$QTDIR/lib/libqcad2.a
    LIBS += $$QTDIR/lib/libqca-ossld2.a
    LIBS += -ldnsapi -lssl32 -leay32
    DEFINES += DELIBERATE_DEBUG=1
  } 
  CONFIG(release, debug|release) {
    LIBS += ../qxmpp-0.2.0/lib/libqxmpp.a
    LIBS += $$QTDIR/lib/libqca2.a
    LIBS += $$QTDIR/lib/libqca-ossl2.a
    LIBS += -ldnsapi -lssl32 -leay32
  }
}
message ("using extra libs $$LIBS")

audio {
  message ("---width audio")
  QT += multimedia
}

mobilaudio {
  message ("---with mobil-audio")
  CONFIG += mobility
  MOBILITY += multimedia
  DEFINES += DO_MOBI_AUDIO=1
} else {
  DEFINES += DO_MOBI_AUDIO=0
}

message ("QT variable is $$QT")

build_x86 {
  QMAKE_LFLAGS += "-z noexecstack"
}

RESOURCES = dchat.qrc

UI_DIR = tmp/$$BUILD_SUB/ui
MOC_DIR = tmp/$$BUILD_SUB/moc
RCC_DIR = tmp/$$BUILD_SUB/rcc
OBJECTS_DIR = tmp/$$BUILD_SUB/obj

FORMS = \
	ui/dchat.ui \
	ui/DebugLog.ui \
        ui/getpassword.ui \
        ui/config-edit.ui \
        ui/socket-display.ui \
        ui/cert-candidate.ui \
        ui/contact-edit.ui \
        ui/pick-string.ui \
        ui/chat-box.ui \
        ui/chat-content.ui \
        ui/helpwin.ui \
        ui/subscription.ui \
        ui/save-as.ui \
        ui/simple-password.ui \
        ui/request-subscribe.ui \
        ui/start-listener.ui \
        ui/list-direct.ui \
        ui/cert-edit.ui \
        ui/cert-input.ui \
	ui/server-account.ui \
	ui/count-down.ui \
	ui/cert-list.ui \
        ui/enter-string.ui \
        ui/edit-simple.ui \


SOURCES = \
        src/main.cpp \
	src/dchat.cpp \
        src/dchat-magic.cpp \
	src/cmdoptions.cpp \
	src/delib-debug.cpp \
        src/deliberate.cpp \
        src/version.cpp \
        src/config-edit.cpp \
        src/direct-listener.cpp \
        src/direct-caller.cpp \
        src/symmetric-socket.cpp \
        src/pick-cert.cpp \
        src/cert-store.cpp \
        src/pick-string.cpp \
        src/chat-box.cpp \
        src/chat-content.cpp \
        src/helpview.cpp \
        src/html-mangle.cpp \
        src/subscription-change.cpp \
        src/simple-pass.cpp \
        src/contact-list-model.cpp \
        src/xegal-client.cpp \
        src/add-listener.cpp \
	src/account-edit.cpp \
	src/direct-message.cpp \
	src/direct-parser.cpp \
	src/cert-list-edit.cpp \
        src/enter-string.cpp \
        src/edit-simple.cpp \
        src/name-list-model.cpp \
        src/qml-view.cpp \
    src/xlogin-model.cpp \
    src/xcontact-model.cpp





audio {
	SOURCES += src/audio-message.cpp
}

mobilaudio {
	SOURCES += src/mobi-audi-message.cpp
}



HEADERS += \
	include/dchat.h \
	include/dchat-magic.h \
	include/cmdoptions.h \
	include/delib-debug.h \
        include/deliberate.h \
        include/version.h \
        include/config-edit.h \
        include/direct-listener.h \
        include/direct-caller.h \
        include/symmetric-socket.h \
        include/pick-cert.h \
        include/cert-types.h \
        include/cert-store.h \
        include/pick-string.h \
        include/chat-box.h \
        include/chat-content.h \
        include/server-contact.h \
        include/helpview.h \
        include/html-mangle.h \
        include/subscription-change.h \
        include/simple-pass.h \
        include/contact-list-model.h \
        include/xegal-client.h \
        include/add-listener.h \
	include/account-edit.h \
	include/direct-message.h \
	include/direct-parser.h \
	include/cert-list-edit.h \
        include/enter-string.h \
        include/edit-simple.h \
        include/name-list-model.h \
        include/qml-view.h \
    include/xlogin-model.h \
    include/xcontact-model.h

audio {
	HEADERS += include/audio-message.h
}

mobilaudio {
	HEADERS += include/mobi-audi-message.h
}

gencert {
  SOURCES += src/cert-generate.cpp 
  HEADERS += include/cert-generate.h
}










