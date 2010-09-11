
/****************************************************************
 * This outFile is distributed under the following license:
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
#include <QAudioOutput>
#include <QTimer>
#include <QDebug>

namespace egalite
{

AudioMessage::AudioMessage (QWidget *parent)
  :QDialog (parent),
   parentWidget (parent),
   record (0),
   player (0),
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

AudioMessage::~AudioMessage ()
{
  outFile.close ();
  inFile.close ();
  if (record) {
    disconnect (record, 0,0,0);
    record->stop ();
    delete record;
    record = 0;
  }
  if (player) {
    disconnect (player, 0,0,0);
    player->stop ();
    delete player;
    player = 0;
  }
}

void
AudioMessage::Record (const QPoint & where, const QSize & size)
{
  QString tmpdir = QDesktopServices::storageLocation 
                    (QDesktopServices::CacheLocation);
  tmpdir = ".";
  filename = tmpdir + QDir::separator() 
                    + QString ("audio-egalite-out.raw");
  outFile.setFileName(filename);
  outFile.open( QIODevice::WriteOnly | QIODevice::Truncate );
  
  outFormat.setFrequency(8000);
  outFormat.setChannels(1);
  outFormat.setSampleSize(16);
  outFormat.setCodec("audio/pcm");
  outFormat.setByteOrder(QAudioFormat::LittleEndian);
  outFormat.setSampleType(QAudioFormat::SignedInt);

  QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
  qDebug () << " try to set outFormat";
  if (!info.isFormatSupported(outFormat)) {
    qWarning()<<"default outFormat not supported try to use nearest";
    outFormat = info.nearestFormat(outFormat);
  }
  if (record == 0) {
    record = new QAudioInput(outFormat, this);
  }
  qDebug () << " record ";
  qDebug () << " rate " << record->format().frequency();
  record->reset ();
  record->start(&outFile);
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
    qDebug () << " closing outFile " << outFile.fileName();
    qDebug () << "         size " << outFile.size ();
    outFile.close ();
    qDebug () << " recording stopped ";
    
    emit HaveAudio ();
    #if 0
    encoder.SetParams (outFormat);
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

void
AudioMessage::StartPlay ()
{
  qDebug () << "Play audio message";
  inFile.open (QFile::ReadOnly);
  if (player == 0) {
    player = new QAudioOutput (inFormat, this);
    connect (player, SIGNAL (stateChanged (QAudio::State)),
             this, SLOT (PlayChanged (QAudio::State)));
  }
  emit PlayStarting ();
  player->start (&inFile);
}

void
AudioMessage::PlayChanged (QAudio::State state)
{
  if (state == QAudio::IdleState) {
    StopPlay ();
  }
}

void
AudioMessage::StopPlay ()
{
  inFile.close ();
  if (player) {
    player->stop();
  }
  busyReceive = false;
  emit PlayFinished ();
  qDebug () << " done playing audio message";
}

int
AudioMessage::Size ()
{
  return outFile.size ();
}

void
AudioMessage::StartReceive ()
{
  QString tmpdir = QDesktopServices::storageLocation 
                    (QDesktopServices::CacheLocation);
  tmpdir = ".";
  QString filename = tmpdir + QDir::separator() 
                    + QString ("audio-egalite-in.raw");
  inFile.setFileName(filename);
  busyReceive = true;
}

void
AudioMessage::FinishReceive ()
{
  StartPlay ();
}

} // namespace

