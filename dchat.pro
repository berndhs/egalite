#
# dchat 
#

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


SOURCES = src/main.cpp \
	src/dchat.cpp \
	src/cmdoptions.cpp \
	src/delib-debug.cpp \



HEADERS = \
	src/dchat.h \
	src/cmdoptions.h \
	src/delib-debug.h \


