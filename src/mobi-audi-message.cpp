
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

#include "mobi-audi-message.h"
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QUuid>
#include <QDebug>

namespace egalite
{

MobiAudiMessage::MobiAudiMessage (QWidget *parent)
  :QDialog (parent),
   parentWidget (parent),
   audioSource (0),
   recorder (0),
   player (0),
   recTime (10.0),
   tick (0.0),
   secsLeft (0.0),
   playLimitTimer (0)
{
  audioSource = new QAudioCaptureSource (this);
  recorder = new QMediaRecorder (audioSource, this);
  player = new QMediaPlayer (this);
  ui.setupUi (this);
  playLimitTimer = new QTimer (this);
  hide ();
  clock.start ();
  inStateText[0] = QString("Stopper");
  inStateText[1] = QString("Recording");
  inStateText[2] = QString("Paused");
  inStateText[3] = QString("unknown");
}

MobiAudiMessage::~MobiAudiMessage ()
{
  outFile.close ();
  inFile.close ();
  if (recorder) {
    disconnect (recorder, 0,0,0);
    recorder->stop ();
    delete recorder;
    recorder = 0;
  }
  if (player) {
    disconnect (player, 0,0,0);
    player->stop ();
    delete player;
    player = 0;
  }
}

void
MobiAudiMessage::Record (const QPoint & where, const QSize & size)
{
  Q_UNUSED (size)
  QString tmpdir = QDesktopServices::storageLocation 
                    (QDesktopServices::CacheLocation);
  QDir tmppath (tmpdir);
  if (!tmppath.exists()) {
    tmppath.mkpath (tmpdir);
  } 
  filename = tmpdir + QDir::separator() 
                    + QString ("egalite-%1.ogg")
                      .arg(Tempname());
  outFile.setFileName(filename);
  outFile.open( QIODevice::WriteOnly | QIODevice::Truncate );
  
  outFormat.setCodec("audio/speex");
  outFormat.setSampleRate (-1);
  outFormat.setBitRate(96000);
  outFormat.setQuality (QtMultimediaKit::NormalQuality);
  outFormat.setEncodingMode (QtMultimediaKit::ConstantQualityEncoding);
  outFormat.setChannelCount(1);

  recorder->setEncodingSettings (outFormat, 
                                 QVideoEncoderSettings(),
                                 QString("ogg"));
  qDebug () << " record at " << clock.elapsed ();
  recorder->setOutputLocation (QUrl (outFile.fileName()));
  recorder->record ();
  move (parentWidget->mapToGlobal (where));
  show ();
  StartCount (10.0);
  QTimer::singleShot (10000,this, SLOT (StopRecording()));
}

void
MobiAudiMessage::StopRecording ()
{
  qDebug () << " done recording at " << clock.elapsed ();
  if (recorder) {
    qint64 usecs = recorder->duration ();
    recorder->stop ();
    qDebug () << " closing outFile " << outFile.fileName();
    qDebug () << "         size " << outFile.size ();
    outFile.close ();
    qDebug () << " recording stopped ";
    
    emit HaveAudio (usecs);
  }
}

void
MobiAudiMessage::StartCount (double maxtime)
{
  tick = 1.0;
  QTimer::singleShot (1000, this, SLOT (CountDown()));
  secsLeft = maxtime;
  show ();
}

void
MobiAudiMessage::CountDown ()
{
  secsLeft -= tick;
  ui.countDown->display (secsLeft);
  if (recorder) {
    ui.stateLabel->setText (inStateText [recorder->state()]);
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
MobiAudiMessage::StartPlay ()
{
  qDebug () << __PRETTY_FUNCTION__ << "Play audio message";
  if (player == 0) {
    return;
  }
  player->setMedia (QMediaContent (QUrl::fromLocalFile (inFile.fileName())));
  
  emit PlayStarting ();
  player->play ();
  int playtime = (playUSecs / 1000) + 1000;
 // playLimitTimer->start (playtime);
 // QTimer::singleShot (playtime, this, SLOT (StopPlay()));
}

#if 0
void
MobiAudiMessage::CheckPlayState ()
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
#endif


void
MobiAudiMessage::StopPlay ()
{
  inFile.close ();
#if 0
  inFile.remove ();
#endif
  if (player) {
    player->stop();
  }
  busyReceive = false;
  emit PlayFinished ();
  playLimitTimer->stop ();
  qDebug () << " done playing audio message";
}

int
MobiAudiMessage::Size ()
{
  return outFile.size ();
}

void
MobiAudiMessage::StartReceive ()
{
  QString tmpdir = QDesktopServices::storageLocation 
                    (QDesktopServices::CacheLocation);
  QDir tmppath (tmpdir);
  if (!tmppath.exists()) {
    tmppath.mkpath (tmpdir);
  } 
  QString filename = tmpdir + QDir::separator() 
                    + QString ("egalite-%1.ogg")
                      .arg(Tempname ());
  inFile.setFileName(filename);
  busyReceive = true;
}

void
MobiAudiMessage::FinishReceive ()
{
  StartPlay ();
}

QString
MobiAudiMessage::Tempname ()
{
  QString tmp = QUuid::createUuid().toString();
  tmp.remove (0,1);
  tmp.chop (1);
  return tmp;
}

} // namespace

