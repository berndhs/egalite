
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
#include "deliberate.h"

using namespace deliberate;

namespace egalite
{

MobiAudiMessage::MobiAudiMessage (QWidget *parent)
  :QDialog (parent),
   parentWidget (parent),
   audioSource (0),
   recorder (0),
   isRecording (false),
   player (0),
   recTime (10.0),
   tick (0.0),
   secsLeft (0.0),
   normalCodec ("audio/speex")
{
  audioSource = new QAudioCaptureSource (this);
  recorder = new QMediaRecorder (audioSource, this);
  player = new QMediaPlayer (this);
  ui.setupUi (this);
  hide ();
  normalCodec = Settings().value("audio/codec",normalCodec).toString();
  Settings().setValue ("audio/codec",normalCodec);
  clock.start ();
  inStateText[0] = QString("Stopper");
  inStateText[1] = QString("Recording");
  inStateText[2] = QString("Paused");
  inStateText[3] = QString("unknown");
  connect (player, SIGNAL (stateChanged(QMediaPlayer::State)),
           this, SLOT (PlayerStateChanged (QMediaPlayer::State)));
  connect (recorder, SIGNAL (error( QMediaRecorder::Error )),
           this, SLOT (RecorderError ( QMediaRecorder::Error )));
}

MobiAudiMessage::~MobiAudiMessage ()
{
  outFile.close ();
  inFile.close ();
  if (recorder) {
    disconnect (recorder, 0,0,0);
    recorder->stop ();
    isRecording = false;
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
  
  outFormat.setCodec(normalCodec);
  outFormat.setSampleRate (-1);
  outFormat.setBitRate(96000);
  outFormat.setQuality (QtMultimediaKit::NormalQuality);
  outFormat.setEncodingMode (QtMultimediaKit::ConstantQualityEncoding);
  outFormat.setChannelCount(1);

  recorder->setEncodingSettings (outFormat, 
                                 QVideoEncoderSettings(),
                                 QString("ogg"));
  qDebug () << __PRETTY_FUNCTION__ << " record at " << clock.elapsed ();
  recorder->setOutputLocation (QUrl (outFile.fileName()));
  recorder->record ();
  isRecording = true;
  move (parentWidget->mapToGlobal (where));
  show ();
  StartCount (10.0);
  QTimer::singleShot (10000,this, SLOT (StopRecording()));
}

void
MobiAudiMessage::RecorderError  (QMediaRecorder::Error error)
{
  qDebug () << __PRETTY_FUNCTION__ << " error " << error ;
  qDebug() << "               " << recorder->errorString();
  if (error == QMediaRecorder::NoError) {
    return;
  }
  isRecording = false;
  if (recorder) {
    recorder->stop ();
  }
  StopRecording ();
}

void
MobiAudiMessage::StopRecording ()
{
  qDebug () << __PRETTY_FUNCTION__ << " done recording at " << clock.elapsed ();
  if (recorder && isRecording) {
    qint64 usecs = recorder->duration ();
    recorder->stop ();
    isRecording = false;
    qDebug () << " closing outFile " << outFile.fileName();
    qDebug () << "            size " << outFile.size ();
    qDebug () << "        duration " << usecs;
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
  qDebug () << __PRETTY_FUNCTION__ << "Play audio message " << inFile.fileName();
  if (player == 0) {
    return;
  }
  if (playList.isEmpty()) {
    return;
  }
  PlayItem item = playList.takeFirst();
  if (item.filename.isEmpty() || item.duration < 1) {
    return;
  }
  qDebug () << "   readable ? " << QFileInfo (item.filename).isReadable();
  player->setMedia (QMediaContent (QUrl::fromLocalFile (item.filename)));
  
  emit PlayStarting ();
  player->setVolume (50);
  player->setPosition (0);
  player->play ();
  qDebug () << __PRETTY_FUNCTION__ ;
  qDebug () << "    playing  " << player->media().canonicalUrl();
  qDebug () << "  have audio " << player->isAudioAvailable ();
  qDebug () << "    mime     " << player->media().canonicalResource().mimeType();
  qDebug () << "    codec    " << player->media().canonicalResource().audioCodec();
  qDebug () << "    bitrate  " << player->media().canonicalResource().audioBitRate();
  qDebug () << "    null ?   " << player->media().isNull();
  qDebug () << "    duration " << player->duration();
  qDebug () << "    position " << player->position();
  qDebug () << "    err      " << player->errorString();
  qDebug () << "    rate     " << player->playbackRate();
  int playtime = item.duration;
  qDebug () << "    play tiemout set to " << playtime;
  QTimer::singleShot (playtime, this, SLOT (TimeoutPlay()));
}

void
MobiAudiMessage::PlayerError (QMediaPlayer::Error error)
{
  qDebug () << __PRETTY_FUNCTION__ << error;
  qDebug () << "               " << player->errorString();
}


void
MobiAudiMessage::PlayerStateChanged (QMediaPlayer::State state)
{
  qDebug () << __PRETTY_FUNCTION__ << " new state " << state;
  qDebug () << "    playing  " << player->media().canonicalUrl();
  qDebug () << "  have audio " << player->isAudioAvailable ();
  qDebug () << "    mime     " << player->media().canonicalResource().mimeType();
  qDebug () << "    codec    " << player->media().canonicalResource().audioCodec();
  qDebug () << "    bitrate  " << player->media().canonicalResource().audioBitRate();
  qDebug () << "    null ?   " << player->media().isNull();
  qDebug () << "    duration " << player->duration();
  qDebug () << "    position " << player->position();
  qDebug () << "    err      " << player->errorString();
  qDebug () << "    rate     " << player->playbackRate();
  switch (state) {
  case QMediaPlayer::PlayingState:
    break;
  case QMediaPlayer::PausedState:
    break;
  case QMediaPlayer::StoppedState:
    StopPlay ();
    break;
  }
}

void
MobiAudiMessage::TimeoutPlay ()
{
  qDebug () << __PRETTY_FUNCTION__ ;
  if (player && player->state() != QMediaPlayer::StoppedState) {
    StopPlay();
  }
}


void
MobiAudiMessage::StopPlay ()
{
  inFile.close ();
  inFile.remove ();
  if (player) {
    player->stop();
  }
  busyReceive = false;
  emit PlayFinished ();
  qDebug () << __PRETTY_FUNCTION__ << " done playing audio message";
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
MobiAudiMessage::FinishReceive (const QString & filename, qint64 duration)
{
  qDebug () << __PRETTY_FUNCTION__ << filename << duration;
  playList.append (PlayItem (filename, duration));
  QTimer::singleShot (100, this, SLOT (StartPlay ()));
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

