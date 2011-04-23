#include "irc-text-browser.h"


/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
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


#include <QDebug>
#include <QTimer>

namespace egalite
{

IrcTextBrowser::IrcTextBrowser (QGraphicsItem *prnt)
  :QGraphicsTextItem(prnt)
{
  setTextInteractionFlags (Qt::TextSelectableByMouse
                         | Qt::TextSelectableByKeyboard
                         | Qt::LinksAccessibleByMouse
                         | Qt::LinksAccessibleByKeyboard);
  setObjectName ("IrcTextBrowser_");
  connect (this, SIGNAL (linkActivated(const QString &)),
           this, SLOT (doActivateLink(const QString &)));
  qDebug () << __PRETTY_FUNCTION__ << this << " parent " << prnt;
}

QRectF
IrcTextBrowser::boundingRect () const
{
  return QGraphicsTextItem::boundingRect ();
}

int
IrcTextBrowser::zero ()
{
  return 0;
}

void
IrcTextBrowser::adjustSize ()
{
  QGraphicsTextItem::adjustSize ();
}

bool
IrcTextBrowser::event (QEvent *evt)
{
  qDebug () << __PRETTY_FUNCTION__ << evt->type() << evt;
  int t = evt->type ();
  if (t == QEvent::GraphicsSceneResize) {
    qDebug () << __PRETTY_FUNCTION__ << "          graphics resize ";
  } else if (t == QEvent::Resize) {
    qDebug () << __PRETTY_FUNCTION__ << "  plain resize ";
  }
  QGraphicsTextItem::event (evt);
}

qreal
IrcTextBrowser::getHeight () const
{
  qreal h = boundingRect().height();
  qDebug () << __PRETTY_FUNCTION__ << h;
  return h;
}

qreal
IrcTextBrowser::getWidth () const
{
  qreal w = boundingRect().width();
  qDebug () << __PRETTY_FUNCTION__ << w;
  return w;
}

QString
IrcTextBrowser::getName () const
{
  return objectName();
}

void
IrcTextBrowser::setName (const QString & name)
{
  setObjectName (QString ("IrcTextBrowser_") + name);
}


void
IrcTextBrowser::setTextWidth (qreal wid)
{
  QGraphicsTextItem::setTextWidth (wid);
  qDebug () << __PRETTY_FUNCTION__ << objectName() << " set Width " << wid 
            << " is now " << textWidth();
  emit heightChanged (getHeight());
  emit widthChanged (getWidth());
}

void
IrcTextBrowser::setHtml (const QString & html)
{
  qDebug () << __PRETTY_FUNCTION__ << objectName() << " set html " << html;
  QGraphicsTextItem::setHtml (html);
  qDebug () << "           textWidth " << textWidth();
  emit heightChanged (getHeight());
}

void
IrcTextBrowser::doActivateLink (const QString & link)
{
  qDebug () << __PRETTY_FUNCTION__ << objectName() << " link activated, re-emit " << link;
  emit activatedLink (link);
}



} // namespace