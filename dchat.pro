
# dchat 
#
#//
#//  Copyright (C) 2010 - Bernd H Stramm 
#//
#// This program is distributed under the terms of 
#// the GNU General Public License version 3 
#//
#// This software is distributed in the hope that it will be useful, 
#// but WITHOUT ANY WARRANTY; without even the implied warranty 
#// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
#//


TEMPLATE = app
MAKEFILE = MakeDChat
TARGET = bin/egalite
unix {
  CONFIG += debug
  CONFIG += crypto
}
win32 {
  CONFIG += debug_and_release
  CONFIG += link_prl
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


unix {
  message ("Applying Unix settings")
  !include ( options.pri ) {
    INCLUDEPATH += /usr/include/qxmpp
    #INCLUDEPATH += /usr/include/QtMultimediaKit
    message ("Added to default include; $$INCLUDEPATH")
    LIBS += -l$$QXMPP_NAME
    DEFINES += DO_AUDIO=0
    message ("No options.pri, using default $$INCLUDEPATH")
  } else {
    INCLUDEPATH += $$QXMPP_BASE/include/qxmpp
    #INCLUDEPATH += /usr/include/QtMultimediaKit
    message ("Added to optional include; $$INCLUDEPATH")
    DEFINES += DO_AUDIO=$$DO_AUDIO
    exists ("$$QXMPP_BASE/lib64") {
      LIBS += -L$$QXMPP_BASE/lib64 -l$$QXMPP_NAME
    } else { 
      LIBS += -L$$QXMPP_BASE/lib -l$$QXMPP_NAME
    }
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
message ("QT variable is $$QT")

build_x86 {
  QMAKE_LFLAGS += "-z noexecstack"
}

RESOURCES = dchat.qrc

UI_DIR = tmp/ui
MOC_DIR = tmp/moc
RCC_DIR = tmp/rcc
OBJECTS_DIR = tmp/obj

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
        ui/irc-qml-channel-group.ui \
        ui/irc-control.ui \
        ui/irc-qml-control.ui \
        ui/irc-float.ui \
        ui/irc-account.ui \
        ui/enter-string.ui \
        ui/edit-simple.ui \


SOURCES = \
        src/main.cpp \
	src/dchat.cpp \
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
        src/cert-generate.cpp \
	src/account-edit.cpp \
	src/direct-message.cpp \
	src/direct-parser.cpp \
	src/cert-list-edit.cpp \
        src/qml-irc-channel-group.cpp \
        src/irc-abstract-channel.cpp \
        src/irc-float.cpp \
        src/irc-control.cpp \
        src/irc-qml-control.cpp \
        src/irc-socket.cpp \
        src/irc-ctcp.cpp \
        src/irc-sock-static.cpp \
        src/irc-qml-sock-static.cpp \
        src/irc-nick-edit.cpp \
        src/irc-text-browser.cpp \
        src/irc-known-server-model.cpp \
        src/enter-string.cpp \
        src/edit-simple.cpp \
        src/user-list-model.cpp \

audio {
	SOURCES += src/audio-message.cpp
}



HEADERS += \
	src/dchat.h \
	src/cmdoptions.h \
	src/delib-debug.h \
        src/deliberate.h \
        src/version.h \
        src/config-edit.h \
        src/direct-listener.h \
        src/direct-caller.h \
        src/symmetric-socket.h \
        src/pick-cert.h \
        src/cert-types.h \
        src/cert-store.h \
        src/pick-string.h \
        src/chat-box.h \
        src/chat-content.h \
        src/server-contact.h \
        src/helpview.h \
        src/html-mangle.h \
        src/subscription-change.h \
        src/simple-pass.h \
        src/contact-list-model.h \
        src/xegal-client.h \
        src/add-listener.h \
        src/cert-generate.h \
	src/account-edit.h \
	src/direct-message.h \
	src/direct-parser.h \
	src/cert-list-edit.h \
        src/qml-irc-channel-group.h \
        src/irc-abstract-channel.h \
        src/irc-control.h \
        src/irc-qml-control.h \
        src/irc-socket.h \
        src/irc-ctcp.h \
        src/irc-sock-static.h \
        src/irc-qml-sock-static.h \
        src/irc-float.h \
        src/irc-nick-edit.h \
        src/irc-text-browser.h \
        src/irc-known-server-model.h \
        src/enter-string.h \
        src/edit-simple.h \
        src/user-list-model.h \

audio {
	HEADERS += src/audio-message.h
}


