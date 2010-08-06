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

#include "dchat.h"

#include <QApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTranslator>
#include <QLocale>
#include <QTextCodec>
#include <QtCrypto>
#include <QXmppLogger.h>
#include <QDebug>
#include "delib-debug.h"
#include "cmdoptions.h"
#include "deliberate.h"
#include "version.h"

namespace deliberate {

void
SetStyle (QSettings &zett)
{
  QStringList avail = QStyleFactory::keys();
  QString     normal("oxygen");
  normal = zett.value ("style/windowstyle",normal).toString();
  if (normal == "gtk+") {
    qDebug () << "Windows style " << normal << " is broken, not supported";
    return;
  }
  if (avail.contains (normal, Qt::CaseInsensitive)) {
    QApplication::setStyle (normal);
    zett.setValue ("style/windowstyle",normal);
  } else {
    QStyle * pSt = QApplication::style();
    if (pSt) {
      QString defaultname = pSt->objectName();
      zett.setValue ("style/windowstyle",defaultname);
    }
  }
}

} // namespace

int
main (int argc, char* argv[])
{
  QCoreApplication::setApplicationName ("egalite");
  QCoreApplication::setOrganizationName ("BerndStramm");
  QCoreApplication::setOrganizationDomain ("bernd-stramm.com");
  deliberate::ProgramVersion pv ("Egalite");
  QCoreApplication::setApplicationVersion (pv.Version());
  QSettings  settings;
  deliberate::SetSettings (settings);
  settings.setValue ("program",pv.MyName());

  deliberate::SetStyle (settings);

  QCA::Initializer  qcaInit;
  QApplication  app (argc,argv);
  QCA::scanForPlugins ();
  // We need to ensure that we have certificate handling support
  if ( !QCA::isSupported( "cert" ) ) {
    qDebug() << "Sorry, no PKI certificate support" ;
  } else {
    qDebug () << " cert support available ";
  }

  QString locale = QLocale::system().name();
  QTranslator  translate;
  QString xlateFile (QString ("egalite_") + locale);
  QString langDir (":/translate");
  bool found = translate.load (xlateFile, langDir);
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
      bool found = translate.load (xlateFile, langDir);
      QTextCodec::setCodecForTr (QTextCodec::codecForName ("utf8"));
      app.installTranslator (&translate);
    }
  }
  /** the real main program starts here */

  egalite::DChatMain  chatmain;

  chatmain.Init (&app);
  app.setWindowIcon (chatmain.windowIcon());

  chatmain.Run ();
  return app.exec ();
}
