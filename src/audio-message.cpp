
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

#include "audio-message.h"
#include <QDesktopServices>
#include <QDir>
#include <QAudioInput>
#include <QTimer>
#include <QDebug>

namespace egalite
{

AudioMessage::AudioMessage (QWidget *parent)
  :QDialog (parent),
   parentWidget (parent),
   record (0),
   recTime (10.0),
   tick (0.0),
   secsLeft (0.0),
   limitTimer (0)
{
  ui.setupUi (this);
  limitTimer = new QTimer (this);
  ui.countDown->setDigitCount (4);
  ui.countDown->setMode (QLCDNumber::Dec);
  hide ();
}

void
AudioMessage::Record (const QPoint & where, const QSize & size)
{
  QString tmpdir = QDesktopServices::storageLocation 
                    (QDesktopServices::CacheLocation);
  tmpdir = ".";
  filename = tmpdir + QDir::separator() 
                    + QString ("audio-egalite.raw");
  file.setFileName(filename);
  file.open( QIODevice::WriteOnly | QIODevice::Truncate );
  
  format.setFrequency(8000);
  format.setChannels(1);
  format.setSampleSize(16);
  format.setCodec("audio/pcm");
  format.setByteOrder(QAudioFormat::LittleEndian);
  format.setSampleType(QAudioFormat::SignedInt);

  QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
  qDebug () << " try to set format";
  if (!info.isFormatSupported(format)) {
    qWarning()<<"default format not supported try to use nearest";
    format = info.nearestFormat(format);
  }
  if (record == 0) {
    record = new QAudioInput(format, this);
  }
  qDebug () << " record ";
  qDebug () << " rate " << record->format().frequency();
  record->reset ();
  record->start(&file);
  connect (limitTimer, SIGNAL (timeout()), this, SLOT (CountDown()));
  move (parentWidget->mapToGlobal (where));
  resize (size);
  show ();
  StartCount (10.0);
}

void
AudioMessage::StopRecording ()
{
  qDebug () << " done recording";
  if (record) {
    record->stop ();
    qDebug () << " closing file " << file.fileName();
    qDebug () << "         size " << file.size ();
    file.close ();
    qDebug () << " recording stopped ";
    
    emit HaveAudio ();
    #if 0
    encoder.SetParams (format);
    encoder.Encode (outputFile.fileName(), QString ("./testdata.ogg"));
    #endif
  }
}

void
AudioMessage::StartCount (double maxtime)
{
  tick = 0.1;
  limitTimer->start (100);
  secsLeft = maxtime;
  show ();
  CountDown ();
}

void
AudioMessage::CountDown ()
{
  ui.countDown->display (secsLeft);
  secsLeft -= tick;
  qDebug () << " time left " << secsLeft;
qDebug () << " countDonw pos " << pos() << " shown " << ui.countDown->isVisible();
  if (secsLeft <= 0.0) {
qDebug () << " callign stop with " << secsLeft << " left ";
    StopRecording();
    limitTimer->stop ();
    ui.countDown->display (0);
    hide ();
    accept ();
  }
}

int
AudioMessage::Size ()
{
  return file.size ();
}

} // namespace

