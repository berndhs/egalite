#ifndef AUDIO_MESSAGE_H
#define AUDIO_MESSAGE_H

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
#include <QAudio>
#include <QAudioFormat>
#include <QFile>
#include <QString>
#include <QPoint>
#include <QLCDNumber>
#include <QWidget>
#include <QDialog>
#include "ui_count-down.h"

class QAudioInput;
class QAudioOutput;
class QTimer;

namespace egalite
{

class AudioMessage : public QDialog
{
Q_OBJECT
public:

  AudioMessage (QWidget * parent=0);
  ~AudioMessage ();

  QString        Filename () { return filename; }
  QAudioFormat & OutFormat () { return outFormat; }
  QAudioFormat & InFormat () { return inFormat; }


  void  SetFilename (QString fn) { filename = fn; }

  QFile * InFile () { return  &inFile; }
  void  StartReceive ();

  bool  BusyReceive () { return busyReceive; }

  void  Record (const QPoint & where, const QSize & size);
  void  StartPlay ();
  int   Size ();

public slots:

  void StopRecording ();
  void FinishReceive ();
  void StopPlay ();

private slots:

  void CountDown ();
  void PlayChanged (QAudio::State state);
  void CheckPlayState ();

signals:

  void HaveAudio ();
  void PlayStarting ();
  void PlayFinished ();

private:

  void   StartCount (double maxtime);

  QWidget       *parentWidget;
  QString        filename;
  QAudioFormat   outFormat;
  QAudioFormat   inFormat;
  QFile          outFile;
  QFile          inFile;
  QAudioInput   *record;
  QAudioOutput  *player;
  double         recTime;
  double         tick;
  double         secsLeft;
  QTimer        *recLimitTimer;
  QTimer        *playLimitTimer;
  bool           busyReceive;

  Ui_CountDownDisplay ui;
} ;

} // namespace

#endif
