
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

QT += core gui network xml xmlpatterns sql webkit

unix:{
  message ("Applying Unix settings")
  INCLUDEPATH += /usr/local/include/qxmpp
  DEFINES += DELIBERATE_DEBUG=1
}
linux-g++:message ("Detecting Linux and gnu C++")
linux-g++-64:message ("Detecting Linux and gnu C++ 64 bit")
linux-g++-32:message ("Detecting Linux and gnu C++ 32 bit")

linux-g++-64:{
  message ("Linux g++ 64 bit")
  LIBS += -L/usr/local/lib64/ -lQXmppClient
}
linux-g++-32:{
  message ("Linux g++ 32 bit")
  LIBS += -L/usr/local/lib/ -lQXmppClient
}
win32:{
  message ("Using Windows 32 bit settings")
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
        ui/helpwin.ui \


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



