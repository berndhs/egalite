#include "delib-debug.h"
#include <stdlib.h>
#include <iostream>
#include <qapplication.h>
#include <QPoint>
#include <QFile>
#include <QFileDialog>

//
//  Copyright (C) 2010 - Bernd H Stramm
//
// This file is distributed under the terms of
// the GNU General Public License version 2
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//


using namespace std;

namespace deliberate
{


static DebugLog *staticLog(0);

void UseMyOwnMessageHandler ()
{
  qInstallMsgHandler (deliberate::MyOwnMessageOutput);
}

void MyOwnMessageOutput (QtMsgType type, const char* msg)
{
#if DELIBERATE_DEBUG
  switch (type) {
  case QtDebugMsg:
    if (staticLog) {
      staticLog->Log ("Qt Debug: ", msg);
    } else {
      cout << "Qt Debug: " << msg << endl;
    }
    break;
  case QtWarningMsg:
    if (staticLog) {
      staticLog->Log ("Qt Warn: ", msg);
    } else {
      cout << "Qt Warn: " << msg << endl;
    }
    break;
  case QtCriticalMsg:
    if (staticLog) {
      staticLog->Log ("Qt Critical: ", msg);
    } else {
      cout << "Qt Critical: " << msg << endl;
    }
    break;
  case QtFatalMsg:
    cout << "Qt Fatal: " << msg << endl;
    if (staticLog) {
      staticLog->Log ("Qt Fatal: ", msg);
    } else {
      cout << "Qt Fatal: " << msg << endl;
    }
    abort();
    break;
  default:
    cout << " unknown Qt msg type: " << msg << endl;
    if (staticLog) {
      staticLog->Log ("Qt Debug: ", msg);
    } else {
      cout << "Qt Debug: " << msg << endl;
    }
    break;
  }
#else
  switch (type) {
  case QtFatalMsg:
    cout << "Qt Fatal: " << msg << endl;
    abort();
    break;
  case QtDebugMsg:
  case QtWarningMsg:
  case QtCriticalMsg:
  default:
    // start prayer, maybe it's not a problem
    break;
  }
#endif

}


void
StartDebugLog (bool gui)
{
  if (staticLog == 0) {
    staticLog = new DebugLog ;
  }
  staticLog->StartLogging ();
  staticLog->UseGui (gui);
  if (gui) {
    staticLog->move (QPoint (0,0));
    staticLog->show ();
  }
}

void
StartFileLog (QString filename)
{
  if (staticLog == 0) {
    staticLog = new DebugLog ;
  }
  staticLog->StartLogging ();
  staticLog->LogToFile (filename);
}

void
StopDebugLog ()
{
  if (staticLog) {
    staticLog->StopLogging ();
    staticLog->hide ();
  }
}

bool
DebugLogRecording ()
{
  if (staticLog) {
    return staticLog->IsLogging();
  } else {
    return false;
  }
}


DebugLog::DebugLog ()
  :QDialog(0),
   isLogging (false),
   logToFile (false)
{
  setupUi (this);
  Connect ();
  hide ();
}

DebugLog::DebugLog (QWidget * parent)
  :QDialog (parent),
   isLogging (false),
   logToFile (false)
{
  setupUi (this);
  Connect ();
  hide ();
}

DebugLog::~DebugLog ()
{
  logFile.close ();
}

void
DebugLog::Connect ()
{
  connect (closeButton, SIGNAL (clicked()), this, SLOT(Close()));
  connect (stopButton, SIGNAL (clicked()), this, SLOT (StopLogging()));
  connect (startButton, SIGNAL (clicked()), this, SLOT (StartLogging()));
  connect (saveButton, SIGNAL (clicked()), this, SLOT (SaveLog()));
}

void
DebugLog::Close ()
{
  isLogging = false;
  hide ();
}

void
DebugLog::quit ()
{
  Close ();
}

void
DebugLog::closeEvent (QCloseEvent *event)
{
  Close ();
}

bool
DebugLog::Log (const char* msg)
{
  if (isLogging && useGui) {
    logBox->append (QString(msg));
    update ();
  }
  if (logToFile) {
    logFile.write (QByteArray (msg));
    logFile.write ("\n");
    logFile.flush ();
  }
  return isLogging;
}

bool
DebugLog::Log (const char* kind, const char* msg)
{
  QString realMessage (QString(kind) + " - " + QString(msg));
  if (isLogging && useGui) {
    logBox->append (QString(kind) + " - " + QString(msg));
    update ();
  }
  if (logToFile) {
    logFile.write (realMessage.toUtf8());
    logFile.write ("\n");
    logFile.flush ();
  }
  return isLogging;
}

void
DebugLog::SaveLog ()
{
  QString saveFile = QFileDialog::getSaveFileName (this, tr("Save Log File"),
                     "./debug-log.log",
                     tr("Text Files (*.log *.txt );; All Files (*.*)"));
  if (saveFile.length() > 0) {
    QFile file(saveFile);
    file.open (QFile::WriteOnly);
    file.write (logBox->toPlainText().toLocal8Bit());
    file.close ( );
  }
}

void
DebugLog::LogToFile (QString filename)
{
  logFile.setFileName (filename);
  bool isopen = logFile.open (QFile::WriteOnly);
  logToFile = isopen;

  cout << " log to file " << filename.toStdString() << endl;
  cout << " log file open is " << isopen << endl;
}

} // namespace
