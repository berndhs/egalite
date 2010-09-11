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
#include <QAudioFormat>
#include <QFile>
#include <QString>

class QAudioInput;
class QTimer;

namespace egalite
{

class AudioMessage : public QObject
{
Q_OBJECT
public:

  AudioMessage (QObject * parent=0);

  QAudioFormat & Format () { return format; }

  QString        Filename () { return filename; }

  void  SetFilename (QString fn) { filename = fn; }

  void  Record ();
  int   Size ();

public slots:

  void StopRecording ();

private slots:

  void CountDown ();

signals:

  void HaveAudio ();

private:

  void   StartCount (double maxtime);

  QString        filename;
  QAudioFormat   format;
  QFile          file;
  QAudioInput   *record;
  double         recTime;
  double         tick;
  double         secsLeft;
  QTimer        *limitTimer;
} ;

} // namespace

#endif
