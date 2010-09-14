
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
CONFIG += debug
CONFIG += crypto
TRANS_DIR = translate
TRANSLATIONS += $$TRANS_DIR/egalite_fr.ts $$TRANS_DIR/egalite_de.ts
message ("translations in $$TRANS_DIR/")

CODEFORTR = UTF-8

message ("generating MAKEFILE as $$MAKEFILE")
QT += core gui network xml xmlpatterns sql webkit multimedia

DEFINES += DELIBERATE_QTM1=$$QT_MAJOR_VERSION
DEFINES += DELIBERATE_QTM2=$$QT_MINOR_VERSION
DEFINES += DELIBERATE_QTP=$$QT_PATCH_VERSION


unix {
  message ("Applying Unix settings")
  !include ( options.pri ) {
    INCLUDEPATH += /usr/include/qxmpp
    LIBS += -l$$QXMPP_NAME
    message ("Now options.pri, using default $$INCLUDEPATH")
  } else {
    INCLUDEPATH += $$QXMPP_BASE/include/qxmpp
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
  message ("Applying Windows 32 bit settings")
  INCLUDEPATH += ../qxmpp/source
debug {
  LIBS += ../qxmpp/source/debug/libQXmppClient_d.a
} else {
  LIBS += ../qxmpp/source/debug/libQXmppClient.a
}
}
message ("using extra libs $$LIBS")

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


SOURCES = src/main.cpp \
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
        src/link-mangle.cpp \
        src/subscription-change.cpp \
        src/simple-pass.cpp \
        src/contact-list-model.cpp \
        src/xegal-client.cpp \
        src/add-listener.cpp \
        src/cert-generate.cpp \
	src/account-edit.cpp \
	src/direct-message.cpp \
	src/direct-parser.cpp \
	src/audio-message.cpp \



HEADERS = \
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
        src/link-mangle.h \
        src/subscription-change.h \
        src/simple-pass.h \
        src/contact-list-model.h \
        src/xegal-client.h \
        src/add-listener.h \
        src/cert-generate.h \
	src/account-edit.h \
	src/direct-message.h \
	src/direct-parser.h \
	src/audio-message.h \



