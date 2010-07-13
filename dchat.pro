
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

QT += core gui network xml xmlpatterns sql

unix:{
  INCLUDEPATH += /usr/local/include/qxmpp
  LIBS += -L/usr/local/lib/qxmpp -lQXmppClient
  DEFINES += DELIBERATE_DEBUG=1
}
win32:{
  INCLUDEPATH += ../qxmpp/source
  LIBS += ../qxmpp/source/debug/libQXmppClient_d.a
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
        ui/mirror.ui \
        ui/cert-candidate.ui \
        ui/cert-store.ui \
        ui/contact-edit.ui \
        ui/pick-string.ui \
        ui/chat-box.ui \
        ui/chat-content.ui \


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



