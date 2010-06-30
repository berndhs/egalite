#
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
TARGET = bin/dchat

QT += core gui network

unix:{
  INCLUDEPATH += /usr/local/include/qxmpp
  LIBS += -L/usr/local/lib/qxmpp -lQXmppClient
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


SOURCES = src/main.cpp \
	src/dchat.cpp \
	src/cmdoptions.cpp \
	src/delib-debug.cpp \



HEADERS = \
	src/dchat.h \
	src/cmdoptions.h \
	src/delib-debug.h \


