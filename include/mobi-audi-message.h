#ifndef EGALITE_MOBI_AUDI_MESSAGE_H
#define EGALITE_MOBI_AUDI_MESSAGE_H

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

#include <QObject>
#include <QMediaRecorder>
#include <QMediaPlayer>
#include <QAudioCaptureSource>
#include <QFile>
#include <QString>
#include <QPoint>
#include <QLCDNumber>
#include <QWidget>
#include <QDialog>
#include <QTime>
#include "ui_count-down.h"

class QTimer;

namespace egalite
{

class MobiAudiMessage : public QDialog
{
Q_OBJECT
public:

  MobiAudiMessage (QWidget * parent=0);
  ~MobiAudiMessage ();

  QString        Filename () { return filename; }
  QAudioEncoderSettings & OutFormat () { return outFormat; }
  QAudioEncoderSettings & InFormat () { return inFormat; }


  void  SetFilename (QString fn) { filename = fn; }

  QFile * InFile () { return  &inFile; }
  void  StartReceive ();

  bool  BusyReceive () { return busyReceive; }

  void  Record (const QPoint & where, const QSize & size);
  int   Size ();

  void  SetInLength (qint64 usecs) { playUSecs = usecs; }

public slots:

  void StopRecording ();
  void FinishReceive (const QString & filename, qint64 duration);

private slots:

  void CountDown ();
  void PlayerStateChanged (QMediaPlayer::State state);
  void RecorderError  (QMediaRecorder::Error error);
  void PlayerError  (QMediaPlayer::Error error);
  void StartPlay ();
  void StopPlay ();
  void TimeoutPlay ();

signals:

  void HaveAudio (qint64 usecs);
  void PlayStarting ();
  void PlayFinished ();

private:

  class PlayItem {
  public:
    PlayItem ()
      :filename(QString()), duration(0)
    {}
    PlayItem (const QString & name, qint64 len)
      :filename (name), duration (len)
    {}
  
    QString  filename;
    qint64   duration;
  };

  void       StartCount (double maxtime);
  QString    Tempname ();

  QWidget       *parentWidget;
  QString        filename;

  QAudioCaptureSource   *audioSource;
  QMediaRecorder        *recorder;
  bool                   isRecording;
  QMediaPlayer          *player;
  QAudioEncoderSettings  outFormat;
  QAudioEncoderSettings  inFormat;

  QFile          outFile;
  QFile          inFile;
  double         recTime;
  double         tick;
  double         secsLeft;
  qint64         playUSecs;
  bool           busyReceive;
  QTime          clock;
  QString        inStateText[4];

  QList<PlayItem>  playList;

  QString        normalCodec;

  Ui_CountDownDisplay ui;
} ;

} // namespace

#endif
