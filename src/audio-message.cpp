
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
#include <QTimer>
#include <QUuid>
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
   playLimitTimer (0)
{
  ui.setupUi (this);
  playLimitTimer = new QTimer (this);
  hide ();
  clock.start ();
  inStateText[0] = QString("active");
  inStateText[1] = QString("suspend");
  inStateText[2] = QString("stopped");
  inStateText[3] = QString("idle");
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
  QDir tmppath (tmpdir);
  if (!tmppath.exists()) {
    tmppath.mkpath (tmpdir);
  } 
  filename = tmpdir + QDir::separator() 
                    + QString ("egalite-%1.raw")
                      .arg(Tempname());
  outFile.setFileName(filename);
  outFile.open( QIODevice::WriteOnly | QIODevice::Truncate );
  
  outFormat.setFrequency(22050);
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
  qDebug () << " record at " << clock.elapsed ();
  qDebug () << " rate " << record->format().frequency();
  record->reset ();
  record->setBufferSize (11000);
  record->start(&outFile);
  move (parentWidget->mapToGlobal (where));
  show ();
  StartCount (10.0);
  QTimer::singleShot (10000,this, SLOT (StopRecording()));
}

void
AudioMessage::StopRecording ()
{
  qDebug () << " done recording at " << clock.elapsed ();
  if (record) {
    qint64 usecs = record->processedUSecs ();
    record->stop ();
    qDebug () << " closing outFile " << outFile.fileName();
    qDebug () << "         size " << outFile.size ();
    qDebug () << "     buf size " << record->bufferSize();
    outFile.close ();
    qDebug () << " recording stopped ";
    record->deleteLater();
    record = 0;
    
    emit HaveAudio (usecs);
    #if 0
    encoder.SetParams (outFormat);
    encoder.Encode (outputFile.fileName(), QString ("./testdata.ogg"));
    #endif
  }
}

void
AudioMessage::StartCount (double maxtime)
{
  tick = 1.0;
  QTimer::singleShot (1000, this, SLOT (CountDown()));
  secsLeft = maxtime;
  show ();
}

void
AudioMessage::CountDown ()
{
  secsLeft -= tick;
  ui.countDown->display (secsLeft);
  if (record) {
    ui.stateLabel->setText (inStateText [record->state()]);
  } else {
    ui.stateLabel->setText (tr("gone"));
  }
qDebug () << " countdown at " << secsLeft << " elapsed " << clock.elapsed();
  if (secsLeft <= 0.0) {
    StopRecording();
    ui.countDown->display (0);
    hide ();
    accept ();
  } else {
    QTimer::singleShot (1000, this, SLOT (CountDown()));
  }
}

void
AudioMessage::StartPlay ()
{
  qDebug () << "Play audio message";
  QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
  if (!info.isFormatSupported(inFormat)) {
    qWarning()<<"default inFormat not supported try to use nearest";
    inFormat = info.nearestFormat(inFormat);
  }
  inFile.open (QFile::ReadOnly);
  if (player == 0) {
    player = new QAudioOutput (inFormat, this);
    connect (player, SIGNAL (stateChanged (QAudio::State)),
             this, SLOT (PlayChanged (QAudio::State)));
    connect (playLimitTimer, SIGNAL (timeout()),
             this, SLOT (CheckPlayState ()));
  }
  emit PlayStarting ();
  player->start (&inFile);
  int playtime = (playUSecs / 1000) + 1000;
 // playLimitTimer->start (playtime);
  QTimer::singleShot (playtime, this, SLOT (StopPlay()));
}

void
AudioMessage::CheckPlayState ()
{
  static qint64  oldWork (0);
  if (player) {
    QAudio::State  state = player->state ();
    if (state == QAudio::IdleState) {
      StopPlay ();
    } else {
      qint64 workdone = player->processedUSecs ();
      if (workdone <= oldWork) {  // nothing done in 2 secs, shut it down
        StopPlay ();
        qDebug () << " forcing audio player stop";
      } else {
        oldWork = workdone;
      }
    }
  }
}

void
AudioMessage::PlayChanged (QAudio::State state)
{
  qDebug () << " Audio Play state " << state;
  if (state == QAudio::IdleState || state == QAudio::StoppedState) {
    StopPlay ();
  }
}

void
AudioMessage::StopPlay ()
{
  inFile.close ();
  inFile.remove ();
  if (player) {
    player->stop();
    player->deleteLater ();
    player = 0;
  }
  busyReceive = false;
  emit PlayFinished ();
  playLimitTimer->stop ();
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
  QDir tmppath (tmpdir);
  if (!tmppath.exists()) {
    tmppath.mkpath (tmpdir);
  } 
  QString filename = tmpdir + QDir::separator() 
                    + QString ("egalite-%1.raw")
                      .arg(Tempname ());
  inFile.setFileName(filename);
  busyReceive = true;
}

void
AudioMessage::FinishReceive ()
{
  StartPlay ();
}

QString
AudioMessage::Tempname ()
{
  QString tmp = QUuid::createUuid().toString();
  tmp.remove (0,1);
  tmp.chop (1);
  return tmp;
}

} // namespace

