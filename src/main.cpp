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

  QApplication  app (argc,argv);

  deliberate::CmdOptions  opts ("Egalite");
  opts.AddSoloOption ("debug","D","show Debug log window");

  //deliberate::UseMyOwnMessageHandler ();

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

  if (showDebug) {
    deliberate::StartDebugLog ();
  }

  /** the real main program starts here */

  egalite::DChatMain  chatmain;

  chatmain.Init (&app);

  chatmain.Run ();
  app.exec ();

}
