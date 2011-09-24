#include <QApplication>
#include "cmdoptions.h"
/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include <iostream>
#include "dchat.h"
#include "dchat-magic.h"

#include <QApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTranslator>
#include <QLocale>
#include <QTextCodec>

#include <QStringList>
#include <QXmppLogger.h>
#include <QFont>
#include <QDebug>
#include "delib-debug.h"
#include "cmdoptions.h"
#include "deliberate.h"
#include "version.h"

#if EGALITE_GENCERT
  #include <QtCrypto>
#endif


int
main (int argc, char* argv[])
{
  QCoreApplication::setApplicationName ("egalite");
  QCoreApplication::setOrganizationName ("BerndStramm");
  QCoreApplication::setOrganizationDomain ("bernd-stramm.com");
  deliberate::ProgramVersion pv ("Egalite");
  QCoreApplication::setApplicationVersion (pv.Version());

#if EGALITE_GENCERT
  QCA::Initializer  qcaInit;
#endif
  
  QApplication  app (argc,argv);

  QSettings  settings;
  deliberate::SetSettings (settings);
  settings.setValue ("program",pv.MyName());


  QStringList  configMessages;  
  configMessages.append (QObject::tr("Build with Qt %1").arg(QT_VERSION_STR));
  configMessages.append (QObject::tr("Running with Qt %1").arg(qVersion()));
  
  bool qcaGenerateSupported (false);
  
#if EGALITE_GENCERT
  QCA::scanForPlugins ();
  qcaGenerateSupported = QCA::isSupported ("cert");
#endif
  
  // We need to ensure that we have certificate handling support
  if ( qcaGenerateSupported ) {
    configMessages << "No PKI certificate support on this system" ;
  } else {
    configMessages << " Certificate support available ";
  }
  #if DO_AUDIO
    #if DELIBERATE_QT_AUDIO_OK
    configMessages << QString(" Audio enabled with Qt %1") 
                      .arg (DELIBERATE_QT_NUM) ;
    #else
    configMessages << QString(" No Audio Input with Qt %1") 
                       .arg (DELIBERATE_QT_NUM) ;
    #endif
  #else
    #if DO_MOBI_AUDIO
      #if DELIBERATE_QT_AUDIO_OK
      configMessages << QString(" MobilAudio enabled with Qt %1") 
                        .arg (DELIBERATE_QT_NUM) ;
      #else
      configMessages << QString(" No MobilAudio Input with Qt %1") 
                         .arg (DELIBERATE_QT_NUM) ;
      #endif
    #else
    configMessages << QString (" Audio Disabled in Build Configuration");
    #endif
  #endif
  QString locale = QLocale::system().name();
  QTranslator  translate;
  QString xlateFile (QString ("egalite_") + locale);
  QString langDir (":/translate");
  translate.load (xlateFile, langDir);
  QTextCodec::setCodecForTr (QTextCodec::codecForName ("utf8"));
  app.installTranslator (&translate);

  deliberate::CmdOptions  opts ("Egalite");
  opts.AddSoloOption ("debug","D",QObject::tr("show Debug log window"));
  opts.AddStringOption ("logdebug","L",QObject::tr("write Debug log to file"));
  opts.AddStringOption ("lang","l",
                   QObject::tr("language (2-letter lower case)"));

  deliberate::UseMyOwnMessageHandler ();

  bool optsOk = opts.Parse (argc, argv);
  if (!optsOk) {
    opts.Usage ();
    exit(1);
  }
  if (opts.WantHelp ()) {
    opts.Usage ();
    exit (0);
  }
  pv.CLIVersion ();
  for (int cm=0; cm<configMessages.size(); cm++) {
    deliberate::StdOut () << configMessages[cm] << endl;
  }
  if (opts.WantVersion ()) {
    exit (0);
  }
  bool showDebug = opts.SeenOpt ("debug");

  deliberate::StartDebugLog (showDebug);
  bool logDebug = opts.SeenOpt ("logdebug");
  if (logDebug) {
    QString logfile ("/dev/null");
    opts.SetStringOpt ("logdebug",logfile);
    deliberate::StartFileLog (logfile);
  }
  QXmppLogger * xlogger = QXmppLogger::getLogger();
  if (showDebug || logDebug) {
    xlogger->setLoggingType (QXmppLogger::FileLogging);
  } else {
    xlogger->setLoggingType (QXmppLogger::NoLogging);
  }

  if (opts.SeenOpt ("lang")) {
    QString newlocale (locale);
    opts.SetStringOpt ("lang",newlocale);
    if (newlocale != locale) {   
      QString xlateFile (QString ("egalite_") + newlocale);
      QString langDir (":/translate");
      translate.load (xlateFile, langDir);
      QTextCodec::setCodecForTr (QTextCodec::codecForName ("utf8"));
      app.installTranslator (&translate);
    }
  }
  /** the real main program starts here */
  qDebug () << " plugin library paths " <<  QCoreApplication::libraryPaths();

  egalite::DChatMain  chatmain;

  QString defaultFamily = QFont().family ();
  QString fontFamily ("default");
  int pointSize (-1);
  fontFamily = deliberate::Settings().value("style/normalfont",fontFamily)
                   .toString();
  deliberate::Settings().setValue ("style/normalfont",fontFamily);
  if (fontFamily == "default") {
    fontFamily = defaultFamily;
  }
  pointSize = deliberate::Settings().value ("style/normalpointsize",pointSize)
                   .toInt ();
  deliberate::Settings().setValue ("style/normalpointsize",pointSize);
  if (pointSize < 0) {
    pointSize = QFont().pointSize();
  }
  app.setFont (QFont (fontFamily, pointSize));

  chatmain.Init (&app);
  chatmain.setWindowTitle (egalite::Magic::Name);
  app.setWindowIcon (chatmain.windowIcon());

  chatmain.AddConfigMessages (configMessages);

  chatmain.Run ();
  int result = app.exec ();
  qDebug () << " default font was " << defaultFamily << QFont().pointSize();
  qDebug () << " application returns " << result;
}
